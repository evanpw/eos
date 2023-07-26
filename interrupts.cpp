#include "interrupts.h"

#include "boot.h"
#include "estd/assertions.h"
#include "estd/bits.h"
#include "estd/print.h"
#include "io.h"
#include "keyboard.h"
#include "mem.h"
#include "panic.h"
#include "processor.h"
#include "system.h"
#include "timer.h"
#include "trap.h"

InterruptDescriptor::InterruptDescriptor(uint64_t addr, uint8_t flags)
: addr0(bitSlice(addr, 0, 16)),
  cs(SELECTOR_CODE0),
  flags(flags),
  addr1(bitSlice(addr, 16, 32)),
  addr2(bitSlice(addr, 32, 64)) {}

// Interrupt descriptor flags
static constexpr uint8_t ISR_PRESENT = 0x80;
static constexpr uint8_t ISR_INTERRUPT_GATE = 0x0F;
static constexpr uint8_t ISR_TRAP_GATE = 0x0E;

// PIC initialization command words (ICWs)
// Reference: https://pdos.csail.mit.edu/6.828/2017/readings/hardware/8259A.pdf
enum ICW1 : uint8_t {
    ICW1_ICW4 = 0x01,       // ICW4 present
    ICW1_SINGLE = 0x02,     // Single mode (vs. cascade mode)
    ICW1_INTERVAL4 = 0x04,  // Call address interval 4  (vs. 8)
    ICW1_LEVEL = 0x08,      // Level triggered (vs. edge triggered)
    ICW1_INIT = 0x10,       // Initialization
};

enum ICW4 : uint8_t {
    ICW4_8086 = 0x01,      // 8086 mode (vs. MCS-80/85 mode)
    ICW4_AUTO = 0x02,      // Auto EOI (vs. normal EOI)
    ICW4_MASTER = 0x04,    // Master (vs. slave)
    ICW4_BUFFERED = 0x08,  // Buffered mode
    ICW4_SFNM = 0x10,      // Special fully nested
};

// Offset from IRQ# to interrupt vector
static constexpr uint8_t IRQ_OFFSET = 0x20;

InterruptDescriptor* g_idt = nullptr;
IDTRegister g_idtr;

static IRQHandler irqHandlers[16];

void registerIrqHandler(uint8_t idx, IRQHandler handler) {
    ASSERT(irqHandlers[idx] == nullptr);
    irqHandlers[idx] = handler;
}

// Called by the assembly-language IRQ entry points defined in entry.S
extern "C" void irqEntry(uint8_t idx, TrapRegisters& regs) {
    if (irqHandlers[idx] != nullptr) {
        irqHandlers[idx](regs);
    } else {
        println("IRQ {}", idx);
        println("rip: 0x{:X}", regs.rip);
        println("cs: 0x{:X}", regs.cs);
        println("rflags: 0x{:X}", regs.rflags);
        println("rsp: 0x{:X}", regs.rsp);
        println("ss: 0x{:X}", regs.ss);

        // Send end-of-interrupt (EOI) signal to the PIC(s)
        if (idx >= 8) {
            outb(PIC2_COMMAND, 0x20);
        }

        outb(PIC1_COMMAND, 0x20);
    }
}

void handleException(uint8_t vector, const char* name, InterruptFrame* frame,
                     uint64_t errorCode) {
    println("CPU EXCEPTION {} ({})", vector, name);
    println("errorCode: 0x{:X}", errorCode);
    println("rip: 0x{:X}", frame->rip);
    println("cs: 0x{:X}", frame->cs);
    println("rflags: 0x{:X}", frame->rflags);
    println("rsp: 0x{:X}", frame->rsp);
    println("ss: 0x{:X}", frame->ss);
    halt();
}

void handleException(uint8_t vector, const char* name, InterruptFrame* frame) {
    println("CPU EXCEPTION {} ({})", vector, name);
    println("rip: 0x{:X}", frame->rip);
    println("cs: 0x{:X}", frame->cs);
    println("rflags: 0x{:X}", frame->rflags);
    println("rsp: 0x{:X}", frame->rsp);
    println("ss: 0x{:X}", frame->ss);
    halt();
}

#define EXCEPTION_HANDLER_WITH_CODE(idx, name)                         \
    void __attribute__((interrupt))                                    \
    exceptionHandler##idx(InterruptFrame* frame, uint64_t errorCode) { \
        handleException(idx, name, frame, errorCode);                  \
    }

#define EXCEPTION_HANDLER(idx, name)                                               \
    void __attribute__((interrupt)) exceptionHandler##idx(InterruptFrame* frame) { \
        handleException(idx, name, frame);                                         \
    }

EXCEPTION_HANDLER(0, "Division Error")
EXCEPTION_HANDLER(1, "Debug")
EXCEPTION_HANDLER(2, "Non-Maskable Interrupt")
EXCEPTION_HANDLER(3, "Breakpoint")
EXCEPTION_HANDLER(4, "Overflow")
EXCEPTION_HANDLER(5, "Bound Range Exceeded")
EXCEPTION_HANDLER(6, "Invalid Opcode")
EXCEPTION_HANDLER(7, "Device Not Available")
EXCEPTION_HANDLER_WITH_CODE(8, "Double Fault")
EXCEPTION_HANDLER(9, "Coprocessor Segment Overrun")
EXCEPTION_HANDLER_WITH_CODE(10, "Invalid TSS")
EXCEPTION_HANDLER_WITH_CODE(11, "Segment Not Present")
EXCEPTION_HANDLER_WITH_CODE(12, "Stack-Segment Fault")
EXCEPTION_HANDLER_WITH_CODE(13, "General Protection Fault")
EXCEPTION_HANDLER_WITH_CODE(14, "Page Fault")
EXCEPTION_HANDLER(15, "Reserved")
EXCEPTION_HANDLER(16, "x87 Floating-Point Exception")
EXCEPTION_HANDLER_WITH_CODE(17, "Alignment Check")
EXCEPTION_HANDLER(18, "Machine Check")
EXCEPTION_HANDLER(19, "SIMD Floating-Point Exception")
EXCEPTION_HANDLER(20, "Virtualization Exception")
EXCEPTION_HANDLER_WITH_CODE(21, "Control Protection Exception")
EXCEPTION_HANDLER(22, "Reserved")
EXCEPTION_HANDLER(23, "Reserved")
EXCEPTION_HANDLER(24, "Reserved")
EXCEPTION_HANDLER(25, "Reserved")
EXCEPTION_HANDLER(26, "Reserved")
EXCEPTION_HANDLER(27, "Reserved")
EXCEPTION_HANDLER(28, "Hypervisor Injection Exception")
EXCEPTION_HANDLER_WITH_CODE(29, "VMM Communication Exception")
EXCEPTION_HANDLER_WITH_CODE(30, "Security Exception")
EXCEPTION_HANDLER(31, "Reserved")

// Defined in entry.S
extern "C" uint64_t irqEntriesAsm[];

void configurePIC() {
    // ICW1: Edge triggered, call address interval 8, cascade mode, expect ICw4
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    // ICW2: Interrupt vector = IRQ# + IRQ_OFFSET
    outb(PIC1_DATA, IRQ_OFFSET);
    outb(PIC2_DATA, IRQ_OFFSET + 8);

    // ICW3: Slave PIC at IRQ2
    outb(PIC1_DATA, 1 << 2);
    outb(PIC2_DATA, 2);

    // ICW4: 8086 mode, normal EOI, non-buffered, not special fully nested
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // Unmask all IRQs
    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);

    // Mask all IRQs (for now)
    outb(PIC1_DATA, 0xFF - (1 << 2));  // leave IRQ2 enabled for cascading
    outb(PIC2_DATA, 0xFF);

    // Set up (empty) handlers and interrupt descriptors for each IRQ
    for (size_t idx = 0; idx < 16; ++idx) {
        g_idt[IRQ_OFFSET + idx] =
            InterruptDescriptor(irqEntriesAsm[idx], ISR_PRESENT | ISR_TRAP_GATE);
        irqHandlers[idx] = nullptr;
    }
}

#define REGISTER_EXCEPTION(idx)                                       \
    g_idt[idx] = InterruptDescriptor((uint64_t)exceptionHandler##idx, \
                                     ISR_PRESENT | ISR_TRAP_GATE)

void installInterrupts() {
    g_idt = new InterruptDescriptor[256];

    REGISTER_EXCEPTION(0);
    REGISTER_EXCEPTION(1);
    REGISTER_EXCEPTION(2);
    REGISTER_EXCEPTION(3);
    REGISTER_EXCEPTION(4);
    REGISTER_EXCEPTION(5);
    REGISTER_EXCEPTION(6);
    REGISTER_EXCEPTION(7);
    REGISTER_EXCEPTION(8);
    REGISTER_EXCEPTION(9);
    REGISTER_EXCEPTION(10);
    REGISTER_EXCEPTION(11);
    REGISTER_EXCEPTION(12);
    REGISTER_EXCEPTION(13);
    REGISTER_EXCEPTION(14);
    REGISTER_EXCEPTION(15);
    REGISTER_EXCEPTION(16);
    REGISTER_EXCEPTION(17);
    REGISTER_EXCEPTION(18);
    REGISTER_EXCEPTION(19);
    REGISTER_EXCEPTION(20);
    REGISTER_EXCEPTION(21);
    REGISTER_EXCEPTION(22);
    REGISTER_EXCEPTION(23);
    REGISTER_EXCEPTION(24);
    REGISTER_EXCEPTION(25);
    REGISTER_EXCEPTION(26);
    REGISTER_EXCEPTION(27);
    REGISTER_EXCEPTION(28);
    REGISTER_EXCEPTION(29);
    REGISTER_EXCEPTION(30);
    REGISTER_EXCEPTION(31);

    configurePIC();
    g_idtr.addr = (uint64_t)&g_idt[0];
    g_idtr.limit = 256 * sizeof(InterruptDescriptor) - 1;

    // Set up the TSS to allow interrupts from ring3 -> ring0 by pointing
    // rsp0 to the top of a 16KiB kernel stack
    VirtualAddress kernelStackBottom =
        System::mm().physicalToVirtual(System::mm().pageAlloc(4));
    VirtualAddress kernelStackTop = kernelStackBottom + 4 * PAGE_SIZE;
    Processor::tss().rsp0 = kernelStackTop.value;

    Processor::lidt(g_idtr);
    println("Interrupts initialized");
}

void startInterrupts() {
    Processor::enableInterrupts();

    // Unmask all IRQs
    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);
}
