#include "processor.h"

#include <stdint.h>
#include <string.h>

#include "boot.h"
#include "estd/bits.h"
#include "estd/print.h"
#include "panic.h"

struct CPUIDResult {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
};

static CPUIDResult cpuid(uint32_t func) {
    CPUIDResult result;
    asm volatile("cpuid"
                 : "=a"(result.eax), "=b"(result.ebx), "=c"(result.ecx), "=d"(result.edx)
                 : "a"(func)
                 :);

    return result;
}

struct __attribute__((packed)) SegmentDescriptor {
    uint16_t limit0_16 = 0;
    uint16_t base0_16 = 0;
    uint8_t base16_24 = 0;
    uint8_t type : 4 = 0;
    uint8_t nonSystem : 1 = 0;
    uint8_t dpl : 2 = 0;
    uint8_t present : 1 = 0;
    uint8_t limit16_20 : 4 = 0;
    uint8_t reserved : 1 = 0;
    uint8_t longMode : 1 = 0;
    uint8_t db : 1 = 0;
    uint8_t granularity : 1 = 0;
    uint8_t base24_32 = 0;

    static SegmentDescriptor null() { return SegmentDescriptor(); }

    static SegmentDescriptor code(uint8_t dpl) {
        SegmentDescriptor desc;
        desc.type = 0b1000;  // executable
        desc.nonSystem = 1;
        desc.dpl = dpl;
        desc.present = 1;
        desc.longMode = 1;
        return desc;
    }

    static SegmentDescriptor data(uint8_t dpl) {
        SegmentDescriptor desc;
        desc.type = 0b0010;  // writeable
        desc.nonSystem = 1;
        desc.dpl = dpl;
        desc.present = 1;
        return desc;
    }

    static SegmentDescriptor tss(PhysicalAddress base, uint32_t limit) {
        SegmentDescriptor desc(bitSlice(base.value, 0, 32), limit);
        desc.type = 0b1001;  // available 64-bit TSS
        desc.present = 1;
        return desc;
    }

    static SegmentDescriptor raw(uint64_t value) {
        SegmentDescriptor desc;
        memcpy(&desc, &value, 8);
        return desc;
    }

    SegmentDescriptor() = default;

    SegmentDescriptor(uint32_t base, uint32_t limit) {
        base0_16 = bitSlice(base, 0, 16);
        base16_24 = bitSlice(base, 16, 24);
        base24_32 = bitSlice(base, 24, 32);
        limit0_16 = bitSlice(limit, 0, 16);
        limit16_20 = bitSlice(limit, 16, 20);
    }
};

static_assert(sizeof(SegmentDescriptor) == 8);

static GDTRegister gdtr;
static SegmentDescriptor gdt[8];
TaskStateSegment Processor::s_tss;

void Processor::initDescriptors() {
    // Clear the tss, and set the IOPB base address to the end of the TSS
    // (disabled)
    memset(&s_tss, 0, sizeof(s_tss));
    s_tss.iopb = sizeof(s_tss);

    // The first three entries must match the GDT from the bootloader, since we
    // don't reload segment registers after the lgdt
    gdt[0] = SegmentDescriptor::null();
    gdt[1] = SegmentDescriptor::code(0);
    gdt[2] = SegmentDescriptor::data(0);

    gdt[3] = SegmentDescriptor::null();
    gdt[4] = SegmentDescriptor::data(3);
    gdt[5] = SegmentDescriptor::code(3);
    PhysicalAddress tssAddr(reinterpret_cast<uint64_t>(&s_tss));
    gdt[6] = SegmentDescriptor::tss(tssAddr, sizeof(TaskStateSegment) - 1);
    gdt[7] = SegmentDescriptor::raw(bitSlice(tssAddr.value, 32));

    // Load the GDT register
    gdtr.addr = reinterpret_cast<uint64_t>(&gdt[0]);  // must be a physical address
    gdtr.limit = sizeof(gdt) - 1;
    lgdt(gdtr);

    // Load the task register (pointing to the TSS)
    ltr(SELECTOR_TSS);
}

void Processor::init() {
    initDescriptors();
    checkFeatures();
}

void Processor::checkFeatures() {
    // Make sure that the processor supports the cpuid functions we need
    size_t maxFunc = cpuid(0).eax;
    ASSERT(maxFunc >= 1);
    size_t maxExtFunc = cpuid(0x8000'0000).eax;
    ASSERT(maxExtFunc >= 8);

    // Read all of the cpuid info at once
    auto result0 = cpuid(0);
    auto result1 = cpuid(1);
    auto resultExt1 = cpuid(0x80000001);
    auto resultExt2 = cpuid(0x80000002);
    auto resultExt3 = cpuid(0x80000003);
    auto resultExt4 = cpuid(0x80000004);
    auto resultExt8 = cpuid(0x80000008);

    // Vendor id and processor name
    char vendorId[13];
    memcpy(&vendorId[0], &result0.ebx, 4);
    memcpy(&vendorId[4], &result0.edx, 4);
    memcpy(&vendorId[8], &result0.ecx, 4);
    vendorId[12] = '\0';
    char procName[48];
    memcpy(&procName[0], &resultExt2.eax, 4);
    memcpy(&procName[4], &resultExt2.ebx, 4);
    memcpy(&procName[8], &resultExt2.ecx, 4);
    memcpy(&procName[12], &resultExt2.edx, 4);
    memcpy(&procName[16], &resultExt3.eax, 4);
    memcpy(&procName[20], &resultExt3.ebx, 4);
    memcpy(&procName[24], &resultExt3.ecx, 4);
    memcpy(&procName[28], &resultExt3.edx, 4);
    memcpy(&procName[32], &resultExt4.eax, 4);
    memcpy(&procName[36], &resultExt4.ebx, 4);
    memcpy(&procName[40], &resultExt4.ecx, 4);
    memcpy(&procName[44], &resultExt4.edx, 4);
    println("cpu: {} ({})", procName, vendorId);

    // Detailed cpu identification (family/model/stepping)
    uint8_t extFamily = bitSlice(result1.eax, 20, 28);
    uint8_t extModel = bitSlice(result1.eax, 16, 20);
    uint8_t baseFamily = bitSlice(result1.eax, 8, 12);
    uint8_t baseModel = bitSlice(result1.eax, 4, 8);
    uint8_t family = baseFamily < 0x0F ? baseFamily : (baseFamily + extFamily);
    uint8_t model = (baseFamily == 0x0F || baseFamily == 0x06)
                        ? ((extModel << 4) | baseModel)
                        : baseModel;
    uint8_t stepping = bitSlice(result1.eax, 0, 4);
    println("cpu: family={}, model={}, stepping={}", family, model, stepping);

    // Processor features
    print("cpu:");
    if (checkBit(result1.ecx, 31)) print(" hypervisor");
    if (checkBit(result1.ecx, 30)) print(" rdrand");
    if (checkBit(result1.ecx, 29)) print(" f16c");
    if (checkBit(result1.ecx, 28)) print(" avx");
    if (checkBit(result1.ecx, 27)) print(" osxsave");  // xsave enabled by OS
    if (checkBit(result1.ecx, 26)) print(" xsave");
    if (checkBit(result1.ecx, 25)) print(" aes");
    if (checkBit(result1.ecx, 24)) print(" tsc_deadline_timer");
    if (checkBit(result1.ecx, 23)) print(" popcnt");
    if (checkBit(result1.ecx, 22)) print(" movbe");
    if (checkBit(result1.ecx, 21)) print(" x2apic");
    if (checkBit(result1.ecx, 20)) print(" sse4_2");
    if (checkBit(result1.ecx, 19)) print(" sse4_1");
    if (checkBit(result1.ecx, 18)) print(" dca");
    if (checkBit(result1.ecx, 17)) print(" pcid");
    if (checkBit(result1.ecx, 15)) print(" pdcm");
    if (checkBit(result1.ecx, 14)) print(" xtpr");
    if (checkBit(result1.ecx, 13)) print(" cx16");
    if (checkBit(result1.ecx, 12)) print(" fma");
    if (checkBit(result1.ecx, 11)) print(" sdbg");
    if (checkBit(result1.ecx, 10)) print(" cid");
    if (checkBit(result1.ecx, 9)) print(" ssse3");
    if (checkBit(result1.ecx, 8)) print(" tm2");
    if (checkBit(result1.ecx, 7)) print(" est");
    if (checkBit(result1.ecx, 6)) print(" smx");
    if (checkBit(result1.ecx, 5)) print(" vmx");
    if (checkBit(result1.ecx, 4)) print(" ds_cpl");
    if (checkBit(result1.ecx, 3)) print(" monitor");
    if (checkBit(result1.ecx, 2)) print(" dtse64");
    if (checkBit(result1.ecx, 1)) print(" pclmulqdq");
    if (checkBit(result1.ecx, 0)) print(" pni");

    if (checkBit(result1.edx, 31)) print(" pbe");
    if (checkBit(result1.edx, 30)) print(" ia64");
    if (checkBit(result1.edx, 29)) print(" tm");
    if (checkBit(result1.edx, 28)) print(" ht");
    if (checkBit(result1.edx, 27)) print(" ss");
    if (checkBit(result1.edx, 26)) print(" sse2");
    if (checkBit(result1.edx, 25)) print(" sse");
    if (checkBit(result1.edx, 24)) print(" fxsr");
    if (checkBit(result1.edx, 23)) print(" mmx");
    if (checkBit(result1.edx, 22)) print(" acpi");
    if (checkBit(result1.edx, 21)) print(" ds");
    if (checkBit(result1.edx, 19)) print(" clflush");
    if (checkBit(result1.edx, 18)) print(" pn");
    if (checkBit(result1.edx, 17)) print(" pse36");
    if (checkBit(result1.edx, 16)) print(" pat");
    if (checkBit(result1.edx, 15)) print(" cmov");
    if (checkBit(result1.edx, 14)) print(" mca");
    if (checkBit(result1.edx, 13)) print(" pge");
    if (checkBit(result1.edx, 12)) print(" mtrr");
    if (checkBit(result1.edx, 11)) print(" sep");
    if (checkBit(result1.edx, 9)) print(" apic");
    if (checkBit(result1.edx, 8)) print(" cx8");
    if (checkBit(result1.edx, 7)) print(" mce");
    if (checkBit(result1.edx, 6)) print(" pae");
    if (checkBit(result1.edx, 5)) print(" msr");
    if (checkBit(result1.edx, 4)) print(" tsc");
    if (checkBit(result1.edx, 3)) print(" pse");
    if (checkBit(result1.edx, 2)) print(" de");
    if (checkBit(result1.edx, 1)) print(" vme");
    if (checkBit(result1.edx, 0)) print(" fpu");

    if (checkBit(resultExt1.ecx, 29)) print(" mwaitx");
    if (checkBit(resultExt1.ecx, 28)) print(" perfctr_l2");
    if (checkBit(resultExt1.ecx, 27)) print(" ptsc");
    if (checkBit(resultExt1.ecx, 26)) print(" bpext");
    if (checkBit(resultExt1.ecx, 24)) print(" perfctr_nb");
    if (checkBit(resultExt1.ecx, 23)) print(" perfctr_core");
    if (checkBit(resultExt1.ecx, 22)) print(" topoext");
    if (checkBit(resultExt1.ecx, 21)) print(" tbm");
    if (checkBit(resultExt1.ecx, 19)) print(" nodeid_msr");
    if (checkBit(resultExt1.ecx, 17)) print(" tce");
    if (checkBit(resultExt1.ecx, 16)) print(" fma4");
    if (checkBit(resultExt1.ecx, 15)) print(" lwp");
    if (checkBit(resultExt1.ecx, 13)) print(" wdt");
    if (checkBit(resultExt1.ecx, 12)) print(" skinit");
    if (checkBit(resultExt1.ecx, 11)) print(" xop");
    if (checkBit(resultExt1.ecx, 10)) print(" ibs");
    if (checkBit(resultExt1.ecx, 9)) print(" osvw");
    if (checkBit(resultExt1.ecx, 8)) print(" 3dnowprefetch");
    if (checkBit(resultExt1.ecx, 7)) print(" misalignsse");
    if (checkBit(resultExt1.ecx, 6)) print(" sse4a");
    if (checkBit(resultExt1.ecx, 5)) print(" abm");
    if (checkBit(resultExt1.ecx, 4)) print(" cr8_legacy");
    if (checkBit(resultExt1.ecx, 3)) print(" extapic");
    if (checkBit(resultExt1.ecx, 2)) print(" svm");
    if (checkBit(resultExt1.ecx, 1)) print(" cmp_legacy");
    if (checkBit(resultExt1.ecx, 0)) print(" lahf_lm");

    if (checkBit(resultExt1.edx, 31)) print(" 3dnow");
    if (checkBit(resultExt1.edx, 30)) print(" 3dnowext");
    if (checkBit(resultExt1.edx, 29)) print(" lm");
    if (checkBit(resultExt1.edx, 27)) print(" rdtscp");
    if (checkBit(resultExt1.edx, 26)) print(" pdpe1gb");
    if (checkBit(resultExt1.edx, 25)) print(" fxsr_opt");
    if (checkBit(resultExt1.edx, 22)) print(" mmxext");
    if (checkBit(resultExt1.edx, 20)) print(" nx");
    if (checkBit(resultExt1.edx, 19)) print(" mp");
    if (checkBit(resultExt1.edx, 11)) print(" syscall");
    println("");

    // Physical and virtual address sizes in bits
    // uint8_t localApicId = bitSlice(result1.ebx, 24, 32);
    uint8_t linearAddressSize = bitSlice(resultExt8.eax, 8, 16);
    // uint8_t physicalAddressSize = bitSlice(resultExt8.eax, 0, 8);

    // Test for the properties that we're assuming (some are guaranteed given
    // that we've reached this point in the kernel)
    if (!checkBit(result1.edx, 6)) panic("!pae");
    if (!checkBit(result1.edx, 5)) panic("!msr");
    if (!checkBit(result1.edx, 13)) panic("!pge");
    if (!checkBit(resultExt1.edx, 11)) panic("!syscall");
    if (!checkBit(resultExt1.edx, 26)) panic("!pdpe1gb");
    if (!checkBit(resultExt1.edx, 29)) panic("!lm");
    ASSERT(linearAddressSize >= 47);
}
