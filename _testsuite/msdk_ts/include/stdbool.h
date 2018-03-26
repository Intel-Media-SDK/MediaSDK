#ifdef _MSC_VER
/*
MSVC2012 and older don't have stdbool.h required for building Perl 5.20+
*/
#if _MSC_VER < 1800
  #pragma once
  #define bool char

  /* Perl header config.h defines PERL_STATIC_INLINE as follow:
       #define PERL_STATIC_INLINE static __inline__

     It is not valid for MSVC. Correct define:
   */
  #undef PERL_STATIC_INLINE
  #define PERL_STATIC_INLINE static __inline /**/
#endif  // #if _MSC_VER < 1800

#endif  // #ifdef _MSC_VER
