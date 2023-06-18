#include "acpi.h"

#include <string.h>

#include "aml.h"
#include "bits.h"
#include "print.h"
#include "system.h"
#include "units.h"

uint32_t makeSignature(const char* str) {
    uint32_t result;
    memcpy(&result, str, 4);
    return result;
}

uint64_t makeSignature8(const char* str) {
    uint64_t result;
    memcpy(&result, str, 8);
    return result;
}

// Root System Description Pointer (RSDP)
struct __attribute__((packed)) RSDP {
    uint64_t signature;
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdtAddress;
    // TODO: handle revision 2 and its extra fields

    static RSDP* tryCreate(PhysicalAddress address) {
        RSDP* candidate = System::mm().physicalToVirtual(address).ptr<RSDP>();
        if (candidate->verifySignature() && candidate->verifyChecksum()) {
            return candidate;
        } else {
            return nullptr;
        }
    }

    bool verifySignature() { return signature == makeSignature8("RSD PTR "); }

    bool verifyChecksum() {
        uint8_t sum = 0;
        uint8_t* ptr = (uint8_t*)this;
        for (size_t i = 0; i < sizeof(RSDP); ++i) {
            sum += ptr[i];
        }

        return sum == 0;
    }
};

static_assert(sizeof(RSDP) == 20);

// Common header for all ACPI tables
struct __attribute__((packed)) TableHeader {
    uint32_t signature;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    uint64_t oemTableId;
    uint32_t oemRevision;
    uint32_t creatorId;
    uint32_t creatorRevision;

    static TableHeader* tryCreate(PhysicalAddress address,
                                  const char* expected = 0) {
        // TODO: check more completely for a valid memory address
        if (address == 0) {
            return nullptr;
        }

        TableHeader* candidate =
            System::mm().physicalToVirtual(address).ptr<TableHeader>();

        if (expected && candidate->signature != makeSignature(expected)) {
            return nullptr;
        }

        if (!candidate->verifyChecksum()) {
            return nullptr;
        }

        return candidate;
    }

    bool verifyChecksum() {
        // All bytes of the table (including the checksum) must sum to 0
        uint8_t sum = 0;
        uint8_t* ptr = (uint8_t*)this;
        for (size_t i = 0; i < length; ++i) {
            sum += ptr[i];
        }

        return sum == 0;
    }
};

static_assert(sizeof(TableHeader) == 36);

// Find the location of the RSDP
RSDP* findRSDP() {
    // BIOS data area (BDA)
    uint16_t* bda = (uint16_t*)0x400;

    // The 7th word of the BDA gives the real-mode segment of the extended
    // BIOS data area (EBDA)
    PhysicalAddress ebda = 16 * bda[7];

    // Search every 16 bytes of the first 1KiB of the EBDA
    for (PhysicalAddress address = ebda; address < ebda + KiB; address += 16) {
        if (RSDP* rsdp = RSDP::tryCreate(address); rsdp) {
            return rsdp;
        }
    }

    // If that fails, search the BIOS read-only memory between 0xE0000 and
    // 0xFFFFF
    for (PhysicalAddress address = 0xE0000; address < 0x100000; address += 16) {
        if (RSDP* rsdp = RSDP::tryCreate(address); rsdp) {
            return rsdp;
        }
    }

    return nullptr;
}

static void copyString(char* dest, const char* src, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        dest[i] = src[i];
    }

    dest[n] = '\0';
}

void printTableHeader(TableHeader* header) {
    char buffer[64];
    copyString(buffer, (char*)&header->signature, 4);
    println("{}: length {}, revision {}", buffer, header->length,
            header->revision);
}

// Differentiated System Description Table (DSDT)
void parseDSDT(TableHeader* dsdt) {
    // parseAML((uint8_t*)(dsdt + 1), dsdt->length - sizeof(TableHeader));
}

// Fixed ACPI Description Table (FADT)
void parseFADT(TableHeader* fadt) {
    // This table contains a lot of fields, but we only care about the DSDT.
    // The pointer to the DSDT is stored in the second dword after the header
    // in the FADT
    uint32_t dsdtPtr = ((uint32_t*)(fadt + 1))[1];

    if (TableHeader* dsdt = TableHeader::tryCreate(dsdtPtr); dsdt) {
        // printTableHeader(dsdt);
        parseDSDT(dsdt);
    }
}

// Multiple APIC Description Table
void parseMADT(TableHeader* madt) {}

bool parseACPITables() {
    RSDP* rsdp = findRSDP();
    if (!rsdp) {
        return false;
    }

    // println("ACPI tables found: revision {}", rsdp->revision);

    TableHeader* rsdt = TableHeader::tryCreate(rsdp->rsdtAddress);
    if (!rsdt) {
        println("RSDT not found or invalid");
        return false;
    }

    // printTableHeader(rsdt);

    ASSERT((rsdt->length - sizeof(TableHeader)) % 4 == 0);
    size_t numEntries = (rsdt->length - sizeof(TableHeader)) / 4;

    uint32_t* pointers = (uint32_t*)(rsdt + 1);
    for (size_t i = 0; i < numEntries; ++i) {
        TableHeader* table = TableHeader::tryCreate(pointers[i]);
        if (!table) continue;

        // printTableHeader(table);

        if (table->signature == makeSignature("FACP")) {
            parseFADT(table);
        } else if (table->signature == makeSignature("APIC")) {
            parseMADT(table);
        }
    }

    return true;
}

void initACPI() {
    if (!parseACPITables()) {
        println("No ACPI tables found");
    }

    println("ACPI initialized");
}
