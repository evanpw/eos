#pragma once
#include "address.h"

enum class InterruptsFlag {
    Enabled = 0,
    Disabled = 1,
};

struct __attribute__((packed)) IDTRegister {
    uint16_t limit;
    uint64_t addr;
};

struct __attribute__((packed)) GDTRegister {
    uint16_t limit;
    uint64_t addr;
};

struct __attribute__((packed)) TaskStateSegment {
    uint32_t reserved1;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved2;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t iopb;
};

static_assert(sizeof(TaskStateSegment) == 0x68);

// TODO: to support multiple cores, these static methods will have to be
// instance methods
class Processor {
public:
    static void init();
    static void initDescriptors();
    static void checkFeatures();
    static uint64_t flags() {
        uint64_t rflags;
        asm volatile(
            "pushf\n"
            "pop %0\n"
            : "=rm"(rflags)
            :
            : "memory");

        return rflags;
    }

    static void halt() { asm volatile("hlt"); }
    static void pause() { asm volatile("pause"); }

    static void lidt(IDTRegister& idtr) { asm volatile("lidt %0" : : "m"(idtr)); }
    static void lgdt(GDTRegister& gdtr) { asm volatile("lgdt %0" : : "m"(gdtr)); }
    static void ltr(uint16_t selector) { asm volatile("ltr %0" : : "a"(selector)); }

    static void enableInterrupts() { asm volatile("sti"); }
    static void disableInterrupts() { asm volatile("cli"); }
    static bool interruptsEnabled() { return flags() & (1 << 9); }

    // Disables interrupts and returns the previous interrupt state
    static InterruptsFlag saveAndDisableInterrupts() {
        bool ifFlag = interruptsEnabled();
        disableInterrupts();

        return ifFlag ? InterruptsFlag::Enabled : InterruptsFlag::Disabled;
    }

    // Enables interrupts if they were enabled at the corresponding call to
    // save()
    static void restoreInterrupts(InterruptsFlag state) {
        if (state == InterruptsFlag::Enabled) {
            enableInterrupts();
        }
    }

    static void loadCR3(PhysicalAddress pml4) {
        asm volatile("movq %0, %%cr3" : : "r"(pml4.value) : "memory");
    }

    static void flushTLB() {
        asm volatile(
            "movq  %%cr3, %%rax\n\t"
            "movq  %%rax, %%cr3\n\t"
            :
            :
            : "memory", "rax");
    }

    static uint64_t rdmsr(uint64_t msr) {
        uint32_t low, high;

        asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));

        return concatBits(high, low);
    }

    static void wrmsr(uint64_t msr, uint64_t value) {
        uint32_t low = bitSlice(value, 0, 32);
        uint32_t high = bitSlice(value, 32, 64);

        asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
    }

    static TaskStateSegment& tss() { return s_tss; }

private:
    static TaskStateSegment s_tss;
};
