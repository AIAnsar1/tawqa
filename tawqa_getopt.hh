#pragma once


#ifndef AAD110AE_B4A2_4565_8339_376247633893

#include <string_view>

#define AAD110AE_B4A2_4565_8339_376247633893

#ifdef	__cplusplus
extern "C" {
#endif


extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

struct option
{
#if defined (__STDC__) && __STDC__
  const char *name;
#else
  char *name;
#endif
  int has_arg;
  int *flag;
  int val;
};


#define	no_argument		0
#define required_argument	1
#define optional_argument	2

#if defined (__STDC__) && __STDC__
#ifdef __GNU_LIBRARY__

extern int getopt (int argc, char *const *argv, const char *shortopts);
#else /* not __GNU_LIBRARY__ */
extern int getopt ();
#endif /* __GNU_LIBRARY__ */
extern int getopt_long (int argc, char *const *argv, const char *shortopts,const struct option *longopts, int *longind);
extern int getopt_long_only (int argc, char *const *argv,const char *shortopts,const struct option *longopts, int *longind);
extern int _getopt_internal (int argc, char *const *argv,const char *shortopts,const struct option *longopts, int *longind,int long_only);
#else /* not __STDC__ */
extern int getopt ();
extern int getopt_long ();
extern int getopt_long_only ();
extern int _getopt_internal ();
#endif /* __STDC__ */
#ifdef	__cplusplus

}
#endif











































#endif /* AAD110AE_B4A2_4565_8339_376247633893 */
