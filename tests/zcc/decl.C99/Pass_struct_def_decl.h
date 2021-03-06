// Pass_struct_def_decl.h
// (C)2009,2011 Kenneth Boyd, license: MIT.txt
// using singly defined struct

struct good_test {
	int x_factor
} y;

// ringing the changes on extern
extern struct good_test x1;
extern const struct good_test x2;
extern volatile struct good_test x3;
extern const volatile struct good_test x4;
extern volatile const struct good_test x5;

// ringing the changes on static
static struct good_test x6;
static const struct good_test x7;
static volatile struct good_test x8;
static const volatile struct good_test x9;
static volatile const struct good_test x10;

// extern/static not in first position is deprecated, but legal
const extern struct good_test x11;
volatile extern struct good_test x12;
const extern volatile struct good_test x13;
const volatile extern struct good_test x14;
volatile extern const struct good_test x15;
volatile const extern struct good_test x16;
const static struct good_test x17;
volatile static struct good_test x18;
const static volatile struct good_test x19;
const volatile static struct good_test x20;
volatile static const struct good_test x21;
volatile const static struct good_test x22;

// ringing the changes on _Thread_local extern
extern _Thread_local struct good_test x23;
extern _Thread_local const struct good_test x24;
extern _Thread_local volatile struct good_test x25;
extern _Thread_local const volatile struct good_test x26;
extern _Thread_local volatile const struct good_test x27;
_Thread_local extern struct good_test x28;
_Thread_local extern const struct good_test x29;
_Thread_local extern volatile struct good_test x30;
_Thread_local extern const volatile struct good_test x31;
_Thread_local extern volatile const struct good_test x32;

// ringing the changes on _Thread_local static
static _Thread_local struct good_test x33;
static _Thread_local const struct good_test x34;
static _Thread_local volatile struct good_test x35;
static _Thread_local const volatile struct good_test x36;
static _Thread_local volatile const struct good_test x37;
_Thread_local static struct good_test x38;
_Thread_local static const struct good_test x39;
_Thread_local static volatile struct good_test x40;
_Thread_local static const volatile struct good_test x41;
_Thread_local static volatile const struct good_test x42;

// _Thread_local extern not in first two positions is deprecated, but legal
extern const _Thread_local struct good_test x43;
const extern _Thread_local struct good_test x44;
extern volatile _Thread_local struct good_test x45;
volatile extern _Thread_local struct good_test x46;
extern const _Thread_local volatile struct good_test x47;
extern const volatile _Thread_local struct good_test x48;
const extern _Thread_local volatile struct good_test x49;
const extern volatile _Thread_local struct good_test x50;
const volatile extern _Thread_local struct good_test x51;
extern volatile _Thread_local const struct good_test x52;
extern volatile const _Thread_local struct good_test x53;
volatile extern _Thread_local const struct good_test x54;
volatile extern const _Thread_local struct good_test x55;
volatile const extern _Thread_local struct good_test x56;
_Thread_local const extern struct good_test x57;
const _Thread_local extern struct good_test x58;
_Thread_local volatile extern struct good_test x59;
volatile _Thread_local extern struct good_test x60;
_Thread_local const extern volatile struct good_test x61;
_Thread_local const volatile extern struct good_test x62;
const _Thread_local extern volatile struct good_test x63;
const _Thread_local volatile extern struct good_test x64;
const volatile _Thread_local extern struct good_test x65;
_Thread_local volatile extern const struct good_test x66;
_Thread_local volatile const extern struct good_test x67;
volatile _Thread_local extern const struct good_test x68;
volatile _Thread_local const extern struct good_test x69;
volatile const _Thread_local extern struct good_test x70;

// _Thread_local static not in first two positions is deprecated, but legal
static const _Thread_local struct good_test x71;
const static _Thread_local struct good_test x72;
static volatile _Thread_local struct good_test x73;
volatile static _Thread_local struct good_test x74;
static const _Thread_local volatile struct good_test x75;
static const volatile _Thread_local struct good_test x76;
const static _Thread_local volatile struct good_test x77;
const static volatile _Thread_local struct good_test x78;
const volatile static _Thread_local struct good_test x79;
static volatile _Thread_local const struct good_test x80;
static volatile const _Thread_local struct good_test x81;
volatile static _Thread_local const struct good_test x82;
volatile static const _Thread_local struct good_test x83;
volatile const static _Thread_local struct good_test x84;
_Thread_local const static struct good_test x85;
const _Thread_local static struct good_test x86;
_Thread_local volatile static struct good_test x87;
volatile _Thread_local static struct good_test x88;
_Thread_local const static volatile struct good_test x89;
_Thread_local const volatile static struct good_test x90;
const _Thread_local static volatile struct good_test x91;
const _Thread_local volatile static struct good_test x92;
const volatile _Thread_local static struct good_test x93;
_Thread_local volatile static const struct good_test x94;
_Thread_local volatile const static struct good_test x95;
volatile _Thread_local static const struct good_test x96;
volatile _Thread_local const static struct good_test x97;
volatile const _Thread_local static struct good_test x98;


// check ZCC compiler extensions
#if 9<=10000*__ZCC__+100*__ZCC_MINOR__+__ZCC_PATCHLEVEL__
// the type names should have no linkage in C, external linkage in C++
_Static_assert(0==__zcc_linkage(good_test),"good_test has incorrect linkage");

// check external linkage of variables
_Static_assert(2==__zcc_linkage(x1),"x1 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x2),"x2 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x3),"x3 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x4),"x4 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x5),"x5 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x11),"x11 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x12),"x12 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x13),"x13 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x14),"x14 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x15),"x15 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x16),"x16 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x23),"x23 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x24),"x24 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x25),"x25 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x26),"x26 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x27),"x27 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x28),"x28 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x29),"x29 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x30),"x30 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x31),"x31 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x32),"x32 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x43),"x43 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x44),"x44 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x45),"x45 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x46),"x46 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x47),"x47 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x48),"x48 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x49),"x49 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x50),"x50 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x51),"x51 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x52),"x52 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x53),"x53 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x54),"x54 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x55),"x55 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x56),"x56 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x57),"x57 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x58),"x58 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x59),"x59 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x60),"x60 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x61),"x61 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x62),"x62 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x63),"x63 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x64),"x64 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x65),"x65 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x66),"x66 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x67),"x67 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x68),"x68 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x69),"x69 has incorrect linkage");
_Static_assert(2==__zcc_linkage(x70),"x70 has incorrect linkage");

// check internal linkage of variables
_Static_assert(1==__zcc_linkage(x6),"x6 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x7),"x7 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x8),"x8 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x9),"x9 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x10),"x10 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x17),"x17 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x18),"x18 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x19),"x19 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x20),"x20 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x21),"x21 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x22),"x22 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x33),"x33 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x34),"x34 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x35),"x35 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x36),"x36 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x37),"x37 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x38),"x38 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x39),"x39 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x40),"x40 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x41),"x41 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x42),"x42 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x71),"x71 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x72),"x72 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x73),"x73 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x74),"x74 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x75),"x75 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x76),"x76 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x77),"x77 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x78),"x78 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x79),"x79 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x80),"x80 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x81),"x81 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x82),"x82 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x83),"x83 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x84),"x84 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x85),"x85 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x86),"x86 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x87),"x87 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x88),"x88 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x89),"x89 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x90),"x90 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x91),"x91 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x92),"x92 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x93),"x93 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x94),"x94 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x95),"x95 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x96),"x96 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x97),"x97 has incorrect linkage");
_Static_assert(1==__zcc_linkage(x98),"x98 has incorrect linkage");
#endif
