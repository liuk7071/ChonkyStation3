#pragma once

#include <common.hpp>
#include <logger.hpp>
#include <BEField.hpp>
#include <elfio/elfio.hpp>

#include <charconv>

#include <plusaes/plusaes.hpp>


// Circular dependency
class PlayStation3;

static constexpr u64 SELF_TYPE_LV0      = 1;
static constexpr u64 SELF_TYPE_LV1      = 2;
static constexpr u64 SELF_TYPE_LV2      = 3;
static constexpr u64 SELF_TYPE_APP      = 4;
static constexpr u64 SELF_TYPE_ISO      = 5;
static constexpr u64 SELF_TYPE_LDR      = 6;
static constexpr u64 SELF_TYPE_UNK7     = 7;
static constexpr u64 SELF_TYPE_NPDRM    = 8;

class SELFToELF {
public:
    SELFToELF(PlayStation3* ps3) : ps3(ps3) {}
    PlayStation3* ps3;
    
    int makeELF(const fs::path& path, const fs::path& out_path);
    
    struct CFHeader {
        BEField<u32> magic;
        BEField<u32> ver;
        BEField<u16> attr;
        BEField<u16> category;
        BEField<u32> ext_header_size;
        BEField<u64> file_offset;
        BEField<u64> file_size;
    };
    
    struct ExtendedHeader {
        BEField<u64> ext_hdr_ver;
        BEField<u64> program_identification_header_offset;
        BEField<u64> ehdr_offset;
        BEField<u64> phdr_offset;
        BEField<u64> shdr_offset;
        BEField<u64> segment_ext_hdr_offset;
        BEField<u64> ver_hdr_offset;
        BEField<u64> supplemental_hdr_offset;
        BEField<u64> supplemental_hdr_size;
        BEField<u64> padding;
    };
    
    struct ProgramIdentificationHeader {
        BEField<u64> prog_auth_id;
        BEField<u32> prog_vendor_id;
        BEField<u32> prog_type;
        BEField<u64> prog_sceversion;
        BEField<u64> padding;
    };
    
    // Located after ExtendedHeader, encrypted with AES256CBC using system keys,
    // and then with AES128CBC using the NPDRM KLIC.
    struct CFEncryptionRootHeader {
        u8 key[16];
        u8 key_pad[16];
        u8 iv[16];
        u8 iv_pad[16];
    };
    
    struct NPD {
        BEField<u32> magic;
        BEField<u32> ver;
        BEField<u32> drm_type;
        BEField<u32> app_type;
        u8 content_id[0x30];
        u8 disgest[0x10];
        u8 cid_fn_hash[0x10];
        u8 header_hash[0x10];
        BEField<u64> limited_time_start;
        BEField<u64> limited_time_end;
    };
    
    struct SupplementalHeader {
        BEField<u32> type;
        BEField<u32> size;
        BEField<u64> next;  // == 1 if there is another header after this
        NPD npd;            // This can be other things based on the type, but NPD (type == 3) is the only one we care about
    };
    
    struct CertificationHeader {
        BEField<u64> sign_offset;
        BEField<u32> sign_algorithm;
        BEField<u32> cert_entry_num;
        BEField<u32> attr_entry_num;
        BEField<u32> optional_header_size;
        BEField<u64> pad;
    };
    
    struct SegmentCertificationHeader {
        BEField<u64> segment_offset;
        BEField<u64> segment_size;
        BEField<u32> segment_type;
        BEField<u32> segment_id;
        BEField<u32> sign_algorithm;
        BEField<u32> sign_idx;
        BEField<u32> enc_algorithm;
        BEField<u32> key_idx;
        BEField<u32> iv_idx;
        BEField<u32> comp_algorithm;
    };
    
    // ELF structs
    
    struct Elf64_Ehdr {
        BEField<u32> e_magic;
        BEField<u8>  e_class;
        BEField<u8>  e_data;
        BEField<u8>  e_curver;
        BEField<u8>  e_os_abi;
        BEField<u64> e_abi_ver;
        BEField<u16> e_type;
        BEField<u16> e_machine;
        BEField<u32> e_version;
        BEField<u64> e_entry;
        BEField<u64> e_phoff;
        BEField<u64> e_shoff;
        BEField<u32> e_flags;
        BEField<u16> e_ehsize;
        BEField<u16> e_phentsize;
        BEField<u16> e_phnum;
        BEField<u16> e_shentsize;
        BEField<u16> e_shnum;
        BEField<u16> e_shstrndx;
    };
    
    struct Elf64_Phdr {
        BEField<u32> p_type;
        BEField<u32> p_flags;
        BEField<u64> p_offset;
        BEField<u64> p_vaddr;
        BEField<u64> p_paddr;
        BEField<u64> p_filesz;
        BEField<u64> p_memsz;
        BEField<u64> p_align;
    };
    
    struct Elf64_Shdr {
        BEField<u32> sh_name;
        BEField<u32> sh_type;
        BEField<u64> sh_flags;
        BEField<u64> sh_addr;
        BEField<u64> sh_offset;
        BEField<u64> sh_size;     
        BEField<u32> sh_link;
        BEField<u32> sh_info;
        BEField<u64> sh_addralign;
        BEField<u64> sh_entsize;
    };
    
    static constexpr u8 RAP_KEY[0x10] = {
        0x86, 0x9F, 0x77, 0x45, 0xC1, 0x3F, 0xD8, 0x90, 0xCC, 0xF2, 0x91, 0x88, 0xE3, 0xCC, 0x3E, 0xDF
    };
    static constexpr u8 RAP_PBOX[0x10] = {
        0x0C, 0x03, 0x06, 0x04, 0x01, 0x0B, 0x0F, 0x08, 0x02, 0x07, 0x00, 0x05, 0x0A, 0x0E, 0x0D, 0x09
    };
    static constexpr u8 RAP_E1[0x10] = {
        0xA9, 0x3E, 0x1F, 0xD6, 0x7C, 0x55, 0xA3, 0x29, 0xB7, 0x5F, 0xDD, 0xA6, 0x2A, 0x95, 0xC7, 0xA5
    };
    static constexpr u8 RAP_E2[0x10] = {
        0x67, 0xD4, 0x5D, 0xA3, 0x29, 0x6D, 0x00, 0x6A, 0x4E, 0x7C, 0x53, 0x7B, 0xF5, 0x53, 0x8C, 0x74
    };
    static constexpr u8 NP_KLIC_KEY[0x10] = {
        0xF2, 0xFB, 0xCA, 0x7A, 0x75, 0xB0, 0x4E, 0xDC, 0x13, 0x90, 0x63, 0x8C, 0xCD, 0xFD, 0xD1, 0xEE
    };
    
    struct SELFKey {
        u64 version_start = 0;
        u64 version_end = 0;
        u16 revision = 0;
        u32 type = 0;
        std::string erk;
        std::string riv;
        std::string pub;
        std::string priv;
        u32 curve_type;
        
        void getERK (u8* out);
        void getRIV (u8* out);
        void getPUB (u8* out);
        void getPRIV(u8* out);
    };
    
    SELFKey& getSELFKey(u32 type, u16 revision, u64 version);
    
    SELFKey npdrm_keys[16] = {
        {0x0001000000000000, 0x0001000000000000, 0x0001, SELF_TYPE_NPDRM,
            "F9EDD0301F770FABBA8863D9897F0FEA6551B09431F61312654E28F43533EA6B",
            "A551CCB4A42C37A734A2B4F9657D5540",
            "B05F9DA5F9121EE4031467E74C505C29A8E29D1022379EDFF0500B9AE480B5DAB4578A4C61C5D6BF",
            "00040AB47509BED04BD96521AD1B365B86BF620A98",
            0x11},
        {0x0001000000000000, 0x0001000000000000, 0x0002, SELF_TYPE_NPDRM,
            "8E737230C80E66AD0162EDDD32F1F774EE5E4E187449F19079437A508FCF9C86",
            "7AAECC60AD12AED90C348D8C11D2BED5",
            "05BF09CB6FD78050C78DE69CC316FF27C9F1ED66A45BFCE0A1E5A6749B19BD546BBB4602CF373440",
            "",
            0x0A},
        {0x0003003000000000, 0x0003003000000000, 0x0003, SELF_TYPE_NPDRM,
            "1B715B0C3E8DC4C1A5772EBA9C5D34F7CCFE5B82025D453F3167566497239664",
            "E31E206FBB8AEA27FAB0D9A2FFB6B62F",
            "3F51E59FC74D6618D34431FA67987FA11ABBFACC7111811473CD9988FE91C43FC74605E7B8CB732D",
            "",
            0x08},
        {0x0003004200000000, 0x0003004200000000, 0x0004, SELF_TYPE_NPDRM,
            "BB4DBF66B744A33934172D9F8379A7A5EA74CB0F559BB95D0E7AECE91702B706",
            "ADF7B207A15AC601110E61DDFC210AF6",
            "9C327471BAFF1F877AE4FE29F4501AF5AD6A2C459F8622697F583EFCA2CA30ABB5CD45D1131CAB30",
            "00B61A91DF4AB6A9F142C326BA9592B5265DA88856",
            0x16},
        {0x0003004200000000, 0x0003004200000000, 0x0006, SELF_TYPE_NPDRM,
            "8B4C52849765D2B5FA3D5628AFB17644D52B9FFEE235B4C0DB72A62867EAA020",
            "05719DF1B1D0306C03910ADDCE4AF887",
            "2A5D6C6908CA98FC4740D834C6400E6D6AD74CF0A712CF1E7DAE806E98605CC308F6A03658F2970E",
            "",
            0x29},
        {0x0003005000000000, 0x0003005000000000, 0x0007, SELF_TYPE_NPDRM,
            "3946DFAA141718C7BE339A0D6C26301C76B568AEBC5CD52652F2E2E0297437C3",
            "E4897BE553AE025CDCBF2B15D1C9234E",
            "A13AFE8B63F897DA2D3DC3987B39389DC10BAD99DFB703838C4A0BC4E8BB44659C726CFD0CE60D0E",
            "009EF86907782A318D4CC3617EBACE2480E73A46F6",
            0x17},
        {0x0003005000000000, 0x0003005000000000, 0x0009, SELF_TYPE_NPDRM,
            "0786F4B0CA5937F515BDCE188F569B2EF3109A4DA0780A7AA07BD89C3350810A",
            "04AD3C2F122A3B35E804850CAD142C6D",
            "A1FE61035DBBEA5A94D120D03C000D3B2F084B9F4AFA99A2D4A588DF92B8F36327CE9E47889A45D0",
            "",
            0x2A},
        {0x0003005500000000, 0x0003005500000000, 0x000A, SELF_TYPE_NPDRM,
            "03C21AD78FBB6A3D425E9AAB1298F9FD70E29FD4E6E3A3C151205DA50C413DE4",
            "0A99D4D4F8301A88052D714AD2FB565E",
            "3995C390C9F7FBBAB124A1C14E70F9741A5E6BDF17A605D88239652C8EA7D5FC9F24B30546C1E44B",
            "009AC6B22A056BA9E0B6D1520F28A57A3135483F9F",
            0x27},
        {0x0003005500000000, 0x0003005500000000, 0x000C, SELF_TYPE_NPDRM,
            "357EBBEA265FAEC271182D571C6CD2F62CFA04D325588F213DB6B2E0ED166D92",
            "D26E6DD2B74CD78E866E742E5571B84F",
            "00DCF5391618604AB42C8CFF3DC304DF45341EBA4551293E9E2B68FFE2DF527FFA3BE8329E015E57",
            "",
            0x3A},
        {0x0003005600000000, 0x0003005600000000, 0x000D, SELF_TYPE_NPDRM,
            "337A51416105B56E40D7CAF1B954CDAF4E7645F28379904F35F27E81CA7B6957",
            "8405C88E042280DBD794EC7E22B74002",
            "9BFF1CC7118D2393DE50D5CF44909860683411A532767BFDAC78622DB9E5456753FE422CBAFA1DA1",
            "",
            0x18},
        {0x0003005600000000, 0x0003005600000000, 0x000F, SELF_TYPE_NPDRM,
            "135C098CBE6A3E037EBE9F2BB9B30218DDE8D68217346F9AD33203352FBB3291",
            "4070C898C2EAAD1634A288AA547A35A8",
            "BBD7CCCB556C2EF0F908DC7810FAFC37F2E56B3DAA5F7FAF53A4944AA9B841F76AB091E16B231433",
            "",
            0x3B},
        {0x0003006100000000, 0x0003006100000000, 0x0010, SELF_TYPE_NPDRM,
            "4B3CD10F6A6AA7D99F9B3A660C35ADE08EF01C2C336B9E46D1BB5678B4261A61",
            "C0F2AB86E6E0457552DB50D7219371C5",
            "64A5C60BC2AD18B8A237E4AA690647E12BF7A081523FAD4F29BE89ACAC72F7AB43C74EC9AFFDA213",
            "",
            0x27},
        {0x0003006600000000, 0x0003006600000000, 0x0013, SELF_TYPE_NPDRM,
            "265C93CF48562EC5D18773BEB7689B8AD10C5EB6D21421455DEBC4FB128CBF46",
            "8DEA5FF959682A9B98B688CEA1EF4A1D",
            "9D8DB5A880608DC69717991AFC3AD5C0215A5EE413328C2ABC8F35589E04432373DB2E2339EEF7C8",
            "",
            0x18},
        {0x0003007400000000, 0x0003007400000000, 0x0016, SELF_TYPE_NPDRM,
            "7910340483E419E55F0D33E4EA5410EEEC3AF47814667ECA2AA9D75602B14D4B",
            "4AD981431B98DFD39B6388EDAD742A8E",
            "62DFE488E410B1B6B2F559E4CB932BCB78845AB623CC59FDF65168400FD76FA82ED1DC60E091D1D1",
            "",
            0x25},
        {0x0004001100000000, 0x0004001100000000, 0x0019, SELF_TYPE_NPDRM,
            "FBDA75963FE690CFF35B7AA7B408CF631744EDEF5F7931A04D58FD6A921FFDB3",
            "F72C1D80FFDA2E3BF085F4133E6D2805",
            "637EAD34E7B85C723C627E68ABDD0419914EBED4008311731DD87FDDA2DAF71F856A70E14DA17B42",
            "",
            0x24},
        {0x0004004600000000, 0x0004004600000000, 0x001C, SELF_TYPE_NPDRM,
            "8103EA9DB790578219C4CEDF0592B43064A7D98B601B6C7BC45108C4047AA80F",
            "246F4B8328BE6A2D394EDE20479247C5",
            "503172C9551308A87621ECEE90362D14889BFED2CF32B0B3E32A4F9FE527A41464B735E1ADBC6762",
            "",
            0x30}
    };
    
private:
    MAKE_LOG_FUNCTION(log, loader_self);
};
