#include "aml.h"

#include <string.h>

#include "estd/assertions.h"
#include "estd/bits.h"
#include "estd/print.h"

struct StringBuilder {
    void append(char c) {
        ASSERT(index < 64);
        buffer[index++] = c;
    }

    const char* str() {
        if (buffer[index] != '\0') {
            buffer[index] = '\0';
        }

        size_t n = strlen(buffer) + 1;
        char* result = new char[n];
        strncpy(result, buffer, n);
        return result;
    }

    size_t index = 0;
    char buffer[64];
};

namespace ast {

class TermArg {
public:
    enum Type {
        Constant,
        Arg,
    };

    static TermArg* makeConst(uint64_t value) {
        return new TermArg(Constant, value);
    }

    static TermArg* makeArg(uint64_t value) { return new TermArg(Arg, value); }

    void print() {
        if (_type == Constant) {
            ::print("{}", _value);
        } else {
            ASSERT(_type == Arg);
            ::print("Arg{}", _value);
        }
    }

    void printHex() {
        if (_type == Constant) {
            ::print("{:X}", _value);
        } else {
            ASSERT(_type == Arg);
            ::print("Arg{}", _value);
        }
    }

private:
    TermArg(Type type, uint64_t value) : _type(type), _value(value) {}

    Type _type;
    uint64_t _value;
};

}  // namespace ast

class AMLParser {
public:
    AMLParser(uint8_t* code, size_t length)
    : _codeStart(code), _codeEnd(code + length), _current(code) {}

    void parse() { TermList(_codeEnd); }

    void TermList(uint8_t* listEnd) {
        while (_current < listEnd) {
            switch (peek()) {
                case ScopeOp:
                    DefScope();
                    break;

                case ExtOpPrefix:
                    extendedOpcode();
                    break;

                case MethodOp:
                    DefMethod();
                    break;

                case ToHexStringOp:
                    DefToHexString();
                    break;

                default:
                    println("Unsupported opcode in TermList: {:02X}", peek());
                    ASSERT(false);
                    break;
            }
        }
    }

    void extendedOpcode() {
        expect(ExtOpPrefix);

        switch (peek()) {
            case OpRegionOp:
                DefOpRegion();
                break;

            case FieldOp:
                DefField();
                break;

            default:
                println("Unsupported extended opcode: 5B {:02X}", peek());
                ASSERT(false);
                break;
        }
    }

    // DefScope = ScopeOp PkgLength NameString TermList
    void DefScope() {
        expect(ScopeOp);
        uint8_t* listEnd = startPackage();
        const char* name = NameString();
        println("DefScope: name={}", name);
        TermList(listEnd);
    }

    // DefOpRegion = OpRegionOp NameString RegionSpace RegionOffset RegionLen
    void DefOpRegion() {
        expect(OpRegionOp);
        const char* name = NameString();
        uint8_t space = RegionSpace();
        ast::TermArg* offset = RegionOffset();
        ast::TermArg* length = RegionLen();
        print("DefOpRegion: name={}", name);
        print(" space=");
        switch (space) {
            case 0x00:
                print("SystemMemory");
                break;

            case 0x01:
                print("SystemIO");
                break;

            case 0x02:
                print("PCI_Config");
                break;

            case 0x03:
                print("EmbeddedControl");
                break;

            case 0x04:
                print("SMBus");
                break;

            case 0x05:
                print("System CMOS");
                break;

            case 0x06:
                print("PciBarTarget");
                break;

            case 0x07:
                print("IPMI");
                break;

            case 0x08:
                print("GeneralPurposeIO");
                break;

            case 0x09:
                print("GenericSerialBus");
                break;

            case 0x0A:
                print("PCC");
                break;

            default:
                print("{:02X}", space);
                break;
        }
        print(" offset=");
        offset->printHex();
        print(" length=");
        length->printHex();
        println("");
    }

    // RegionSpace = ByteData
    uint8_t RegionSpace() { return consume(); }

    // RegionOffset = TermArg => Integer
    ast::TermArg* RegionOffset() { return TermArg(); }

    // RegionLen = TermArg => Integer
    ast::TermArg* RegionLen() { return TermArg(); }

    // DefField = FieldOp PkgLength NameString FieldFlags FieldList
    void DefField() {
        expect(FieldOp);
        uint8_t* listEnd = startPackage();
        const char* name = NameString();
        uint8_t flags = FieldFlags();
        println("DefField: name={} flags={:08b}", name, flags);
        FieldList(listEnd);
    }

    // FieldFlags = ByteData
    uint8_t FieldFlags() { return consume(); }

    // FieldList = Nothing | <FieldElement FieldList>
    void FieldList(uint8_t* listEnd) {
        while (_current < listEnd) {
            FieldElement();
        }
    }

    // FieldElement = NamedField | ReservedField | AccessField |
    // ExtendedAccessField | ConnectField
    void FieldElement() {
        if (peek() == 0x00) {
            // ReservedField = 0x00 PkgLength
            ASSERT(false);
        } else if (peek() == 0x01) {
            // AccessField = 0x01 AccessType AccessAttrib
            ASSERT(false);
        } else if (peek() == 0x02) {
            // ConnectField = 0x02 (NameString | BufferData)
            ASSERT(false);
        } else if (peek() == 0x03) {
            // ExtendedAccessField = 0x03 AccessType ExtendedAccessAttrib
            // AccessLength
            ASSERT(false);
        } else if (peek() == '_' || (peek() >= 'A' && peek() <= 'Z')) {
            NamedField();
            return;
        }

        println("Invalid field element: {:02X}", peek());
        ASSERT(false);
    }

    // NamedField = NameSeq PkgLength
    void NamedField() {
        // NameSeq = LeadNameChar NameChar NameChar NameChar
        StringBuilder builder;
        builder.append(consume());  // Must be a LeadNameChar
        builder.append(consume());
        builder.append(consume());
        builder.append(consume());

        const char* name = builder.str();
        uint32_t length = PkgLength();

        println("NamedField: name={} length={}", name, length);
    }

    // DefMethod = MethodOp PkgLength NameString MethodFlags TermList
    void DefMethod() {
        expect(MethodOp);
        uint8_t* listEnd = startPackage();
        const char* name = NameString();
        uint8_t flags = MethodFlags();
        println("DefMethod: name={} flags={:08b}", name, flags);
        TermList(listEnd);
    }

    // MethodFlags = ByteData
    uint8_t MethodFlags() { return consume(); }

    // DefToHexString = ToHexStringOp Operand Target
    void DefToHexString() {
        expect(ToHexStringOp);
        /*ast::TermArg* operand = */ Operand();
        Target();
    }

    // Operand = TermArg => Integer
    ast::TermArg* Operand() { return TermArg(); }

    // Target = SuperName | NullName
    const char* Target() {
        if (accept(NullName)) {
            return "";
        } else {
            return SuperName();
        }
    }

    // SuperName = SimpleName | DebugObj | ReferenceTypeOpcode
    const char* SuperName() {
        switch (peek()) {
            case ExtOpPrefix:
                return DebugObj();

            case RefOfOp:
            case DerefOfOp:
            case IndexOp:
                return ReferenceTypeOpcode();

                // default:
                //     return SimpleName();
        }

        ASSERT(false);
    }

    const char* DebugObj() {
        expect(ExtOpPrefix);
        expect(DebugOp);
        return "DebugObj";
    }

    const char* ReferenceTypeOpcode() { ASSERT(false); }

    // TermArg = ExpressionOpcode | DataObject | ArgObj | LocalObj
    ast::TermArg* TermArg() {
        switch (peek()) {
            case ZeroOp: {
                expect(ZeroOp);
                return ast::TermArg::makeConst(0);
            }

            case OneOp: {
                expect(OneOp);
                return ast::TermArg::makeConst(1);
            }

            case BytePrefix: {
                uint8_t value = ByteConst();
                return ast::TermArg::makeConst(value);
            }

            case WordPrefix: {
                uint16_t value = WordConst();
                return ast::TermArg::makeConst(value);
            }

            case DWordPrefix: {
                uint32_t value = DWordConst();
                return ast::TermArg::makeConst(value);
            }

            case QWordPrefix: {
                uint64_t value = QWordConst();
                return ast::TermArg::makeConst(value);
            }

            case Arg0Op:
                return ast::TermArg::makeArg(0);

            case Arg1Op:
                return ast::TermArg::makeArg(1);

            case Arg2Op:
                return ast::TermArg::makeArg(2);

            case Arg3Op:
                return ast::TermArg::makeArg(3);

            case Arg4Op:
                return ast::TermArg::makeArg(4);

            case Arg5Op:
                return ast::TermArg::makeArg(5);

            case Arg6Op:
                return ast::TermArg::makeArg(6);

            default:
                println("Unsupported opcode in TermArg: {:02X}", peek());
                ASSERT(false);
                break;
        }
    }

    uint8_t ByteConst() {
        expect(BytePrefix);
        return consume();
    }

    uint16_t WordConst() {
        expect(WordPrefix);

        uint16_t result = 0;
        for (size_t i = 0; i < 2; ++i) {
            result = (consume() << (8 * i)) | result;
        }

        return result;
    }

    uint32_t DWordConst() {
        expect(DWordPrefix);

        uint32_t result = 0;
        for (size_t i = 0; i < 4; ++i) {
            result = (consume() << (8 * i)) | result;
        }

        return result;
    }

    uint64_t QWordConst() {
        expect(QWordPrefix);

        uint64_t result = 0;
        for (size_t i = 0; i < 8; ++i) {
            result = (consume() << (8 * i)) | result;
        }

        return result;
    }

    // Just parses a PkgLength field without doing any of the _codeEnd
    // manipulations that startPackage does
    uint32_t PkgLength() {
        uint8_t leadByte = consume();

        // The top 2 bits specify the number of bytes to follow. If none,
        // then the package length is contained in the bottom 6 bits
        size_t followBytes = highBits(leadByte, 2);
        if (followBytes == 0) {
            return lowBits(leadByte, 6);
        }

        // The following bytes are given in little-endian order
        uint32_t result = 0;
        for (size_t i = 0; i < followBytes; ++i) {
            result |= (consume() << (8 * i));
        }

        // If there are follow bytes, then the bottom 4 bits of the package
        // length are given by the bottom 4 bits of the lead byte
        return (result << 4) | lowBits(leadByte, 4);
    }

    uint8_t* startPackage() {
        uint8_t* start = _current;
        uint32_t pkgLength = PkgLength();
        return start + pkgLength;
    }

    // <RootChar NamePath> | <PrefixPath | NamePath>
    const char* NameString() {
        StringBuilder builder;

        // Must start with RootChar or zero or more ParentPrefixChars
        if (accept(RootChar)) {
            builder.append(RootChar);
        } else {
            while (accept(ParentPrefixChar)) {
                builder.append(ParentPrefixChar);
            }
        }

        // NamePath = NameSeg | DualNamePath | MultiNamePath | NullName
        if (accept(DualNamePrefix)) {
            // DualNamePath
            ASSERT(false);
        } else if (accept(MultiNamePrefix)) {
            // MultiNamePath
            ASSERT(false);
        } else if (accept(NullName)) {
            // NullName
        } else {
            // NameSeg, always 4 characters (padded with underscores)
            builder.append(consume());  // Must be a LeadNameChar
            builder.append(consume());
            builder.append(consume());
            builder.append(consume());
        }

        return builder.str();
    }

private:
    enum Opcode : uint8_t {
        NullName = 0x00,
        ZeroOp = 0x00,
        OneOp = 0x01,
        AliasOp = 0x06,
        NameOp = 0x08,
        BytePrefix = 0x0A,
        WordPrefix = 0x0B,
        DWordPrefix = 0x0C,
        StringPrefix = 0x0D,
        QWordPrefix = 0x0E,
        ScopeOp = 0x10,
        BufferOp = 0x11,
        PackageOp = 0x12,
        VarPackageOp = 0x13,
        MethodOp = 0x14,
        ExternalOp = 0x15,
        DualNamePrefix = 0x2E,
        MultiNamePrefix = 0x2F,
        ExtOpPrefix = 0x5B,
        RootChar = 0x5C,
        ParentPrefixChar = 0x5E,
        NameChar = 0x5F,
        Local0Op = 0x60,
        Local1Op = 0x61,
        Local2Op = 0x62,
        Local3Op = 0x63,
        Local4Op = 0x64,
        Local5Op = 0x65,
        Local6Op = 0x66,
        Local7Op = 0x67,
        Local8Op = 0x68,
        Arg0Op = 0x68,
        Arg1Op = 0x69,
        Arg2Op = 0x6A,
        Arg3Op = 0x6B,
        Arg4Op = 0x6C,
        Arg5Op = 0x6D,
        Arg6Op = 0x6E,
        StoreOp = 0x70,
        RefOfOp = 0x71,
        AddOp = 0x72,
        ConcatOp = 0x73,
        SubtractOp = 0x74,
        IncrementOp = 0x75,
        DecrementOp = 0x76,
        MultiplyOp = 0x77,
        DivideOp = 0x78,
        ShiftLeftOp = 0x79,
        ShiftRightOp = 0x7A,
        AndOp = 0x7B,
        NandOp = 0x7C,
        OrOp = 0x7D,
        NorOp = 0x7E,
        XorOp = 0x7F,
        NotOp = 0x80,
        FindSetLeftBitOp = 0x81,
        FindSetRightBitOp = 0x82,
        DerefOfOp = 0x83,
        ConcatResOp = 0x84,
        ModOp = 0x85,
        NotifyOp = 0x86,
        SizeOfOp = 0x87,
        IndexOp = 0x88,
        MatchOp = 0x89,
        CreateDWordFieldOp = 0x8A,
        CreateWordFieldOp = 0x8B,
        CreateByteFieldOp = 0x8C,
        CreateBitFieldOp = 0x8D,
        ObjectTypeOp = 0x8E,
        CreateQWordFieldOp = 0x8F,
        LandOp = 0x90,
        LorOp = 0x91,
        LnotOp = 0x92,
        LEqualOp = 0x93,
        LGreaterOp = 0x94,
        LLessOp = 0x95,
        ToBufferOp = 0x96,
        ToDecimalStringOp = 0x97,
        ToHexStringOp = 0x98,
        ToIntegerOp = 0x99,
        ToStringOp = 0x9C,
        CopyObjectOp = 0x9D,
        MidOp = 0x9E,
        ContinueOp = 0x9F,
        IfOp = 0xA0,
        ElseOp = 0xA1,
        WhileOp = 0xA2,
        NoopOp = 0xA3,
        ReturnOp = 0xA4,
        BreakOp = 0xA5,
        BreakPointOp = 0xCC,
        OnesOp = 0xFF,
    };

    // Prefixed with ExtOpPrefix == 0x5B
    enum ExtOpcode {
        MutexOp = 0x01,
        EventOp = 0x02,
        CondRefOfOp = 0x12,
        CreateFieldOp = 0x13,
        LoadTableOp = 0x1F,
        LoadOp = 0x20,
        StallOp = 0x21,
        SleepOp = 0x22,
        AcquireOp = 0x23,
        SignalOp = 0x24,
        WaitOp = 0x25,
        ResetOp = 0x26,
        ReleaseOp = 0x27,
        FromBCDOp = 0x28,
        ToBCD = 0x29,
        RevisionOp = 0x30,
        DebugOp = 0x31,
        FatalOp = 0x32,
        TimerOp = 0x33,
        OpRegionOp = 0x80,
        FieldOp = 0x81,
        DeviceOp = 0x82,
        PowerResOp = 0x84,
        ThermalZoneOp = 0x85,
        IndexFieldOp = 0x86,
        BankFieldOp = 0x87,
        DataRegionOp = 0x88,
    };

    void expect(uint8_t value) {
        if (_current == _codeEnd) {
            println("expected {:02X}, got EOF", value);
            ASSERT(_current < _codeEnd);
        } else if (*_current != value) {
            println("expected {:02X}, got {:02X}", value, *_current);
            ASSERT(*_current == value);
        } else {
            ++_current;
        }
    }

    bool accept(uint8_t value) {
        if (_current == _codeEnd || *_current != value) {
            return false;
        } else {
            ++_current;
            return true;
        }
    }

    uint8_t consume() {
        uint8_t value = peek();
        ++_current;
        return value;
    }

    uint8_t peek() {
        ASSERT(_current < _codeEnd);
        return *_current;
    }

    uint8_t* _codeStart;
    uint8_t* _codeEnd;
    uint8_t* _current;
};

void parseAML(uint8_t* code, size_t length) {
    AMLParser parser(code, length);
    parser.parse();
}
