#ifndef UNIQUELIBRARY_GLOBAL_H
#define UNIQUELIBRARY_GLOBAL_H

#if defined(MIAMUNIQUELIBRARY_LIBRARY)
#undef MIAMUNIQUELIBRARY_LIBRARY
#define MIAMUNIQUELIBRARY_LIBRARY Q_DECL_EXPORT
#else
#define MIAMUNIQUELIBRARY_LIBRARY Q_DECL_IMPORT
#endif

#endif // UNIQUELIBRARY_GLOBAL_H
