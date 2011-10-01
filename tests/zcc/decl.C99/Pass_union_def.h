// Pass_union_def.h
// (C)2009,2011 Kenneth Boyd, license: MIT.txt
// using singly defined union

union good_test {
	int x_factor
};

// ringing the changes on extern
extern union good_test x1;
extern const union good_test x2;
extern volatile union good_test x3;
extern const volatile union good_test x4;
extern volatile const union good_test x5;

// ringing the changes on static
static union good_test x6;
static const union good_test x7;
static volatile union good_test x8;
static const volatile union good_test x9;
static volatile const union good_test x10;

// extern/static not in first position is deprecated, but legal
const extern union good_test x11;
volatile extern union good_test x12;
const extern volatile union good_test x13;
const volatile extern union good_test x14;
volatile extern const union good_test x15;
volatile const extern union good_test x16;
const static union good_test x17;
volatile static union good_test x18;
const static volatile union good_test x19;
const volatile static union good_test x20;
volatile static const union good_test x21;
volatile const static union good_test x22;

// ringing the changes on _Thread_local extern
extern _Thread_local union good_test x23;
extern _Thread_local const union good_test x24;
extern _Thread_local volatile union good_test x25;
extern _Thread_local const volatile union good_test x26;
extern _Thread_local volatile const union good_test x27;
_Thread_local extern union good_test x28;
_Thread_local extern const union good_test x29;
_Thread_local extern volatile union good_test x30;
_Thread_local extern const volatile union good_test x31;
_Thread_local extern volatile const union good_test x32;

// ringing the changes on _Thread_local static
static _Thread_local union good_test x33;
static _Thread_local const union good_test x34;
static _Thread_local volatile union good_test x35;
static _Thread_local const volatile union good_test x36;
static _Thread_local volatile const union good_test x37;
_Thread_local static union good_test x38;
_Thread_local static const union good_test x39;
_Thread_local static volatile union good_test x40;
_Thread_local static const volatile union good_test x41;
_Thread_local static volatile const union good_test x42;

// _Thread_local extern not in first two positions is deprecated, but legal
extern const _Thread_local union good_test x43;
const extern _Thread_local union good_test x44;
extern volatile _Thread_local union good_test x45;
volatile extern _Thread_local union good_test x46;
extern const _Thread_local volatile union good_test x47;
extern const volatile _Thread_local union good_test x48;
const extern _Thread_local volatile union good_test x49;
const extern volatile _Thread_local union good_test x50;
const volatile extern _Thread_local union good_test x51;
extern volatile _Thread_local const union good_test x52;
extern volatile const _Thread_local union good_test x53;
volatile extern _Thread_local const union good_test x54;
volatile extern const _Thread_local union good_test x55;
volatile const extern _Thread_local union good_test x56;
_Thread_local const extern union good_test x57;
const _Thread_local extern union good_test x58;
_Thread_local volatile extern union good_test x59;
volatile _Thread_local extern union good_test x60;
_Thread_local const extern volatile union good_test x61;
_Thread_local const volatile extern union good_test x62;
const _Thread_local extern volatile union good_test x63;
const _Thread_local volatile extern union good_test x64;
const volatile _Thread_local extern union good_test x65;
_Thread_local volatile extern const union good_test x66;
_Thread_local volatile const extern union good_test x67;
volatile _Thread_local extern const union good_test x68;
volatile _Thread_local const extern union good_test x69;
volatile const _Thread_local extern union good_test x70;

// _Thread_local static not in first two positions is deprecated, but legal
static const _Thread_local union good_test x71;
const static _Thread_local union good_test x72;
static volatile _Thread_local union good_test x73;
volatile static _Thread_local union good_test x74;
static const _Thread_local volatile union good_test x75;
static const volatile _Thread_local union good_test x76;
const static _Thread_local volatile union good_test x77;
const static volatile _Thread_local union good_test x78;
const volatile static _Thread_local union good_test x79;
static volatile _Thread_local const union good_test x80;
static volatile const _Thread_local union good_test x81;
volatile static _Thread_local const union good_test x82;
volatile static const _Thread_local union good_test x83;
volatile const static _Thread_local union good_test x84;
_Thread_local const static union good_test x85;
const _Thread_local static union good_test x86;
_Thread_local volatile static union good_test x87;
volatile _Thread_local static union good_test x88;
_Thread_local const static volatile union good_test x89;
_Thread_local const volatile static union good_test x90;
const _Thread_local static volatile union good_test x91;
const _Thread_local volatile static union good_test x92;
const volatile _Thread_local static union good_test x93;
_Thread_local volatile static const union good_test x94;
_Thread_local volatile const static union good_test x95;
volatile _Thread_local static const union good_test x96;
volatile _Thread_local const static union good_test x97;
volatile const _Thread_local static union good_test x98;

// define-declares
// ringing the changes on extern
extern union good_test1 { int x_factor1; } x_1;
extern const union good_test2 { int x_factor2; } x_2;
extern volatile union good_test3 { int x_factor3; } x_3;
extern const volatile union good_test4 { int x_factor4; } x_4;
extern volatile const union good_test5 { int x_factor5; } x_5;

// ringing the changes on static
static union good_test6 { int x_factor6; } x_6;
static const union good_test7 { int x_factor7; } x_7;
static volatile union good_test8 { int x_factor8; } x_8;
static const volatile union good_test9 { int x_factor9; } x_9;
static volatile const union good_test10 { int x_factor10; } x_10;

// extern/static not in first position is deprecated, but legal
const extern union good_test11 { int x_factor11; } x_11;
volatile extern union good_test12 { int x_factor12; } x_12;
const extern volatile union good_test13 { int x_factor13; } x_13;
const volatile extern union good_test14 { int x_factor14; } x_14;
volatile extern const union good_test15 { int x_factor15; } x_15;
volatile const extern union good_test16 { int x_factor16; } x_16;
const static union good_test17 { int x_factor17; } x_17;
volatile static union good_test18 { int x_factor18; } x_18;
const static volatile union good_test19 { int x_factor19; } x_19;
const volatile static union good_test20 { int x_factor20; } x_20;
volatile static const union good_test21 { int x_factor21; } x_21;
volatile const static union good_test22 { int x_factor22; } x_22;

// ringing the changes on _Thread_local extern
extern _Thread_local union good_test23 { int x_factor23; } x_23;
extern _Thread_local const union good_test24 { int x_factor24; } x_24;
extern _Thread_local volatile union good_test25 { int x_factor25; } x_25;
extern _Thread_local const volatile union good_test26 { int x_factor26; } x_26;
extern _Thread_local volatile const union good_test27 { int x_factor27; } x_27;
_Thread_local extern union good_test28 { int x_factor28; } x_28;
_Thread_local extern const union good_test29 { int x_factor29; } x_29;
_Thread_local extern volatile union good_test30 { int x_factor30; } x_30;
_Thread_local extern const volatile union good_test31 { int x_factor31; } x_31;
_Thread_local extern volatile const union good_test32 { int x_factor32; } x_32;

// ringing the changes on _Thread_local static
static _Thread_local union good_test33 { int x_factor33; } x_33;
static _Thread_local const union good_test34 { int x_factor34; } x_34;
static _Thread_local volatile union good_test35 { int x_factor35; } x_35;
static _Thread_local const volatile union good_test36 { int x_factor36; } x_36;
static _Thread_local volatile const union good_test37 { int x_factor37; } x_37;
_Thread_local static union good_test38 { int x_factor38; } x_38;
_Thread_local static const union good_test39 { int x_factor39; } x_39;
_Thread_local static volatile union good_test40 { int x_factor40; } x_40;
_Thread_local static const volatile union good_test41 { int x_factor41; } x_41;
_Thread_local static volatile const union good_test42 { int x_factor42; } x_42;

// _Thread_local extern not in first two positions is deprecated, but legal
extern const _Thread_local union good_test43 { int x_factor43; } x_43;
const extern _Thread_local union good_test44 { int x_factor44; } x_44;
extern volatile _Thread_local union good_test45 { int x_factor45; } x_45;
volatile extern _Thread_local union good_test46 { int x_factor46; } x_46;
extern const _Thread_local volatile union good_test47 { int x_factor47; } x_47;
extern const volatile _Thread_local union good_test48 { int x_factor48; } x_48;
const extern _Thread_local volatile union good_test49 { int x_factor49; } x_49;
const extern volatile _Thread_local union good_test50 { int x_factor50; } x_50;
const volatile extern _Thread_local union good_test51 { int x_factor51; } x_51;
extern volatile _Thread_local const union good_test52 { int x_factor52; } x_52;
extern volatile const _Thread_local union good_test53 { int x_factor53; } x_53;
volatile extern _Thread_local const union good_test54 { int x_factor54; } x_54;
volatile extern const _Thread_local union good_test55 { int x_factor55; } x_55;
volatile const extern _Thread_local union good_test56 { int x_factor56; } x_56;
_Thread_local const extern union good_test57 { int x_factor57; } x_57;
const _Thread_local extern union good_test58 { int x_factor58; } x_58;
_Thread_local volatile extern union good_test59 { int x_factor59; } x_59;
volatile _Thread_local extern union good_test60 { int x_factor60; } x_60;
_Thread_local const extern volatile union good_test61 { int x_factor61; } x_61;
_Thread_local const volatile extern union good_test62 { int x_factor62; } x_62;
const _Thread_local extern volatile union good_test63 { int x_factor63; } x_63;
const _Thread_local volatile extern union good_test64 { int x_factor64; } x_64;
const volatile _Thread_local extern union good_test65 { int x_factor65; } x_65;
_Thread_local volatile extern const union good_test66 { int x_factor66; } x_66;
_Thread_local volatile const extern union good_test67 { int x_factor67; } x_67;
volatile _Thread_local extern const union good_test68 { int x_factor68; } x_68;
volatile _Thread_local const extern union good_test69 { int x_factor69; } x_69;
volatile const _Thread_local extern union good_test70 { int x_factor70; } x_70;

// _Thread_local static not in first two positions is deprecated, but legal
static const _Thread_local union good_test71 { int x_factor71; } x_71;
const static _Thread_local union good_test72 { int x_factor72; } x_72;
static volatile _Thread_local union good_test73 { int x_factor73; } x_73;
volatile static _Thread_local union good_test74 { int x_factor74; } x_74;
static const _Thread_local volatile union good_test75 { int x_factor75; } x_75;
static const volatile _Thread_local union good_test76 { int x_factor76; } x_76;
const static _Thread_local volatile union good_test77 { int x_factor77; } x_77;
const static volatile _Thread_local union good_test78 { int x_factor78; } x_78;
const volatile static _Thread_local union good_test79 { int x_factor79; } x_79;
static volatile _Thread_local const union good_test80 { int x_factor80; } x_80;
static volatile const _Thread_local union good_test81 { int x_factor81; } x_81;
volatile static _Thread_local const union good_test82 { int x_factor82; } x_82;
volatile static const _Thread_local union good_test83 { int x_factor83; } x_83;
volatile const static _Thread_local union good_test84 { int x_factor84; } x_84;
_Thread_local const static union good_test85 { int x_factor85; } x_85;
const _Thread_local static union good_test86 { int x_factor86; } x_86;
_Thread_local volatile static union good_test87 { int x_factor87; } x_87;
volatile _Thread_local static union good_test88 { int x_factor88; } x_88;
_Thread_local const static volatile union good_test89 { int x_factor89; } x_89;
_Thread_local const volatile static union good_test90 { int x_factor90; } x_90;
const _Thread_local static volatile union good_test91 { int x_factor91; } x_91;
const _Thread_local volatile static union good_test92 { int x_factor92; } x_92;
const volatile _Thread_local static union good_test93 { int x_factor93; } x_93;
_Thread_local volatile static const union good_test94 { int x_factor94; } x_94;
_Thread_local volatile const static union good_test95 { int x_factor95; } x_95;
volatile _Thread_local static const union good_test96 { int x_factor96; } x_96;
volatile _Thread_local const static union good_test97 { int x_factor97; } x_97;
volatile const _Thread_local static union good_test98 { int x_factor98; } x_98;


// check ZCC compiler extensions
#if 9<=10000*__ZCC__+100*__ZCC_MINOR__+__ZCC_PATCHLEVEL__
// the type names should have no linkage in C, external linkage in C++
_Static_assert(0==__zcc_linkage(good_test),"good_test has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test1),"good_test1 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test2),"good_test2 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test3),"good_test3 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test4),"good_test4 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test5),"good_test5 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test6),"good_test6 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test7),"good_test7 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test8),"good_test8 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test9),"good_test9 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test10),"good_test10 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test11),"good_test11 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test12),"good_test12 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test13),"good_test13 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test14),"good_test14 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test15),"good_test15 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test16),"good_test16 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test17),"good_test17 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test18),"good_test18 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test19),"good_test19 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test20),"good_test20 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test21),"good_test21 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test22),"good_test22 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test23),"good_test23 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test24),"good_test24 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test25),"good_test25 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test26),"good_test26 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test27),"good_test27 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test28),"good_test28 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test29),"good_test29 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test30),"good_test30 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test31),"good_test31 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test32),"good_test32 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test33),"good_test33 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test34),"good_test34 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test35),"good_test35 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test36),"good_test36 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test37),"good_test37 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test38),"good_test38 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test39),"good_test39 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test40),"good_test40 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test41),"good_test41 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test42),"good_test42 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test43),"good_test43 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test44),"good_test44 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test45),"good_test45 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test46),"good_test46 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test47),"good_test47 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test48),"good_test48 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test49),"good_test49 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test50),"good_test50 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test51),"good_test51 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test52),"good_test52 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test53),"good_test53 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test54),"good_test54 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test55),"good_test55 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test56),"good_test56 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test57),"good_test57 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test58),"good_test58 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test59),"good_test59 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test60),"good_test60 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test61),"good_test61 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test62),"good_test62 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test63),"good_test63 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test64),"good_test64 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test65),"good_test65 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test66),"good_test66 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test67),"good_test67 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test68),"good_test68 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test69),"good_test69 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test70),"good_test70 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test71),"good_test71 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test72),"good_test72 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test73),"good_test73 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test74),"good_test74 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test75),"good_test75 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test76),"good_test76 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test77),"good_test77 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test78),"good_test78 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test79),"good_test79 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test80),"good_test80 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test81),"good_test81 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test82),"good_test82 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test83),"good_test83 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test84),"good_test84 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test85),"good_test85 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test86),"good_test86 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test87),"good_test87 has incorrect linkage");
_Static_assert(0==__zcc_linkage(good_test88),"good_test88 has incorrect linkage");

// check external linkage of variables
_Static_assert(2==__zcc_linkage(x1),"x1 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_1),"x_1 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x2),"x2 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_2),"x_2 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x3),"x3 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_3),"x_3 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x4),"x4 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_4),"x_4 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x5),"x5 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_5),"x_5 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x11),"x11 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_11),"x_11 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x12),"x12 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_12),"x_12 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x13),"x13 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_13),"x_13 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x14),"x14 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_14),"x_14 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x15),"x15 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_15),"x_15 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x16),"x16 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_16),"x_16 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x23),"x23 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_23),"x_23 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x24),"x24 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_24),"x_24 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x25),"x25 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_25),"x_25 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x26),"x26 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_26),"x_26 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x27),"x27 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_27),"x_27 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x28),"x28 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_28),"x_28 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x29),"x29 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_29),"x_29 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x30),"x30 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_30),"x_30 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x31),"x31 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_31),"x_31 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x32),"x32 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_32),"x_32 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x43),"x43 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_43),"x_43 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x44),"x44 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_44),"x_44 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x45),"x45 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_45),"x_45 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x46),"x46 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_46),"x_46 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x47),"x47 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_47),"x_47 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x48),"x48 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_48),"x_48 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x49),"x49 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_49),"x_49 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x50),"x50 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_50),"x_50 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x51),"x51 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_51),"x_51 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x52),"x52 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_52),"x_52 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x53),"x53 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_53),"x_53 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x54),"x54 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_54),"x_54 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x55),"x55 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_55),"x_55 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x56),"x56 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_56),"x_56 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x57),"x57 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_57),"x_57 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x58),"x58 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_58),"x_58 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x59),"x59 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_59),"x_59 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x60),"x60 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_60),"x_60 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x61),"x61 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_61),"x_61 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x62),"x62 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_62),"x_62 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x63),"x63 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_63),"x_63 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x64),"x64 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_64),"x_64 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x65),"x65 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_65),"x_65 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x66),"x66 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_66),"x_66 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x67),"x67 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_67),"x_67 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x68),"x68 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_68),"x_68 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x69),"x69 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_69),"x_69 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x70),"x70 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x_70),"x_70 has incorrect linkage");

// check internal linkage of variables
_Static_assert(1==__zcc_linkage(x6),"x6 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_6),"x_6 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x7),"x7 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_7),"x_7 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x8),"x8 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_8),"x_8 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x9),"x9 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_9),"x_9 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x10),"x10 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_10),"x_10 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x17),"x17 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_17),"x_17 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x18),"x18 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_18),"x_18 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x19),"x19 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_19),"x_19 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x20),"x20 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_20),"x_20 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x21),"x21 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_21),"x_21 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x22),"x22 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_22),"x_22 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x33),"x33 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_33),"x_33 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x34),"x34 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_34),"x_34 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x35),"x35 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_35),"x_35 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x36),"x36 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_36),"x_36 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x37),"x37 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_37),"x_37 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x38),"x38 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_38),"x_38 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x39),"x39 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_39),"x_39 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x40),"x40 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_40),"x_40 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x41),"x41 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_41),"x_41 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x42),"x42 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_42),"x_42 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x71),"x71 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_71),"x_71 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x72),"x72 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_72),"x_72 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x73),"x73 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_73),"x_73 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x74),"x74 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_74),"x_74 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x75),"x75 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_75),"x_75 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x76),"x76 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_76),"x_76 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x77),"x77 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_77),"x_77 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x78),"x78 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_78),"x_78 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x79),"x79 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_79),"x_79 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x80),"x80 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_80),"x_80 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x81),"x81 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_81),"x_81 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x82),"x82 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_82),"x_82 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x83),"x83 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_83),"x_83 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x84),"x84 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_84),"x_84 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x85),"x85 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_85),"x_85 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x86),"x86 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_86),"x_86 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x87),"x87 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_87),"x_87 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x88),"x88 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_88),"x_88 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x89),"x89 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_89),"x_89 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x90),"x90 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_90),"x_90 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x91),"x91 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_91),"x_91 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x92),"x92 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_92),"x_92 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x93),"x93 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_93),"x_93 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x94),"x94 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_94),"x_94 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x95),"x95 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_95),"x_95 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x96),"x96 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_96),"x_96 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x97),"x97 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_97),"x_97 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x98),"x98 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x_98),"x_98 has incorrect linkage");
#endif
