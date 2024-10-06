#include "interrupts.h"

#include "boot.h"
#include "estd/assertions.h"
#include "estd/bits.h"
#include "estd/print.h"
#include "io.h"
#include "mm.h"
#include "panic.h"
#include "processor.h"
#include "trap.h"

InterruptDescriptor::InterruptDescriptor(uint64_t addr, uint8_t flags)
: addr0(bitSlice(addr, 0, 16)),
  cs(SELECTOR_CODE0),
  flags(flags),
  addr1(bitSlice(addr, 16, 32)),
  addr2(bitSlice(addr, 32, 64)) {}

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

// TODO: should be per-cpu
// This doesn't actually need to be atomic right now, since interrupts are disabled during
// irq handling, but we probably want to change that in the future
AtomicBool irqFlag;

static IRQHandler irqHandlers[16] = {};

void registerIrqHandler(uint8_t irqNo, IRQHandler handler) {
    ASSERT(irqNo < 16);
    ASSERT(!irqHandlers[irqNo]);
    irqHandlers[irqNo] = handler;

    // Unmask this IRQ at the PIC
    uint16_t port = irqNo < 8 ? PIC1_DATA : PIC2_DATA;
    uint8_t mask = inb(port) & ~(1 << (irqNo % 8));
    outb(port, mask);
}

// Called by the assembly-language IRQ entry points defined in entry.S
extern "C" void irqEntry(uint8_t irqNo, TrapRegisters&) {
    ASSERT(irqHandlers[irqNo]);
    ASSERT(!Processor::interruptsEnabled());

    irqFlag.store(true);
    irqHandlers[irqNo](irqNo);
}

void endOfInterrupt(uint8_t irqNo) {
    if (irqNo >= 8) outb(PIC2_COMMAND, EOI);
    outb(PIC1_COMMAND, EOI);

    irqFlag.store(false);
}

void handleException(uint8_t vector, const char* name, TrapRegisters& regs,
                     uint64_t errorCode) {
    println("CPU EXCEPTION {} ({})", vector, name);
    println("errorCode: 0x{:X}", errorCode);
    println("rip: 0x{:X}", regs.rip);
    println("cs: 0x{:X}", regs.cs);
    println("rflags: 0x{:X}", regs.rflags);
    println("rsp: 0x{:X}", regs.rsp);
    println("ss: 0x{:X}", regs.ss);
    halt();
}

void handleException(uint8_t vector, const char* name, TrapRegisters& regs) {
    println("CPU EXCEPTION {} ({})", vector, name);
    println("rip: 0x{:X}", regs.rip);
    println("cs: 0x{:X}", regs.cs);
    println("rflags: 0x{:X}", regs.rflags);
    println("rsp: 0x{:X}", regs.rsp);
    println("ss: 0x{:X}", regs.ss);
    halt();
}

#define EXCEPTION_HANDLER_WITH_CODE(idx, name)                   \
    extern "C" void exceptionHandler##idx(TrapRegisters& regs) { \
        handleException(idx, name, regs, regs.errorCode);        \
    }

#define EXCEPTION_HANDLER(idx, name)                             \
    extern "C" void exceptionHandler##idx(TrapRegisters& regs) { \
        handleException(idx, name, regs);                        \
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
extern "C" uint64_t exceptionEntriesAsm[];

void configurePIC() {
    // ICW1: Edge triggered, call address interval 8, cascade mode, expect ICW4
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

    // Mask all IRQs (for now)
    outb(PIC1_DATA, 0xFF - (1 << 2));  // leave IRQ2 enabled for cascading
    outb(PIC2_DATA, 0xFF);

    // Set up (empty) handlers and interrupt descriptors for each IRQ
    for (size_t idx = 0; idx < 16; ++idx) {
        g_idt[IRQ_OFFSET + idx] =
            InterruptDescriptor(irqEntriesAsm[idx], ISR_PRESENT | ISR_INTERRUPT_GATE);
    }
}

void installInterrupts() {
    ASSERT(!Processor::interruptsEnabled());

    g_idt = new InterruptDescriptor[256];

    for (size_t idx = 0; idx < 32; ++idx) {
        g_idt[idx] = InterruptDescriptor(exceptionEntriesAsm[idx],
                                         ISR_PRESENT | ISR_INTERRUPT_GATE);
    }

    configurePIC();
    g_idtr.addr = (uint64_t)&g_idt[0];
    g_idtr.limit = 256 * sizeof(InterruptDescriptor) - 1;

    new (&irqFlag) AtomicBool;

    // Set up the TSS to allow interrupts from ring3 -> ring0 by pointing
    // rsp0 to the top of a 16KiB kernel stack
    VirtualAddress kernelStackBottom = mm.physicalToVirtual(mm.pageAlloc(4));
    VirtualAddress kernelStackTop = kernelStackBottom + 4 * PAGE_SIZE;
    Processor::tss().rsp0 = kernelStackTop.value;

    Processor::lidt(g_idtr);
    println("pic: init complete");
}

bool inIrq() { return irqFlag.load(); }
