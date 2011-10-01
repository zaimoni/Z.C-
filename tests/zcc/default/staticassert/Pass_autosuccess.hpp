// default\staticassert\Pass_autosuccess.hpp
// ZCC-specific check on extended constant expressions common to C and C++
// (C)2010 Kenneth Boyd, license: MIT.txt

// exercise string literal uses in (extended) integer constant expressions
static_assert("A","automatic success has failed");

static_assert("A"[0],"automatic success has failed");
static_assert(+"A"[0],"automatic success has failed");
static_assert(-"A"[0],"automatic success has failed");
static_assert(!"A"[1],"automatic success has failed");
static_assert(*"A","automatic success has failed");
static_assert(!*"","automatic success has failed");

static_assert("A"+0,"automatic success has failed");
static_assert(0+"A","automatic success has failed");
static_assert("A"+1,"automatic success has failed");
static_assert(1+"A","automatic success has failed");

static_assert("A"-0,"automatic success has failed");

static_assert("A"=="A","automatic success has failed");
static_assert("A"!="B","automatic success has failed");
static_assert("A"!=0,"automatic success has failed");
static_assert(0!="B","automatic success has failed");

static_assert(1==sizeof "","automatic success has failed");
static_assert(sizeof ""==1,"automatic success has failed");
static_assert(2==sizeof "A","automatic success has failed");
static_assert(sizeof "A"==2,"automatic success has failed");

static_assert(1==sizeof *"A","automatic success has failed");
static_assert(sizeof *"A"==1,"automatic success has failed");
static_assert(1==sizeof *"","automatic success has failed");
static_assert(sizeof *""==1,"automatic success has failed");

