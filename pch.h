#pragma once

#if !defined(PCH_H)
#define PCH_H

#define _CRT_DECLARE_NON_STDC_NAMES 0
#define _CRT_SECURE_NO_WARNINGS

#define _WIN32_WINNT _WIN32_WINNT_WIN7

#define WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOSYSMETRICS
#define NOMENUS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOATOM
#define NOCLIPBOARD
#define NODRAWTEXT
#define NOKERNEL
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include <sdkddkver.h>
#include <windows.h>
#include <commctrl.h>
#include <dispatcherqueue.h>
#include <webservices.h>
#include <windowsx.h>
#include <winrt/windows.foundation.h>
#include <winrt/windows.system.h>

#endif