//# execute preloaded function from ~/.cfg
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>

#include "shell.h"
#include "memalloc.h"
#include "output.h"
#include "var.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


extern lua_State *L;

void cbsdlua_error( lua_State *L, const char *fmt, ... ) {
	va_list argp;
	va_start(argp, fmt);
	out2fmt_flush("lua error:");
	vfprintf(stderr, fmt, argp);
	out2fmt_flush("\n");
	va_end(argp);
//	lua_close(L);
//	exit(EXIT_FAILURE);
}

// вызываем функцию на lua
// указываем тип входящих аргументов через d(oble),i(nteger),s(tring)
// символ > отделяет аргументы входащих от результата
//void call_va(lua_State *L, const char *func, const char *sig, ...) {
//int cbsdlua_funccmd(const char *func, const char *sig, ...) {
int cbsdlua_funccmd(int argc, char **argv) {
//const char *func, const char *sig, ...) {
	char *func=argv[1];
	char *sig=argv[2];

	int narg, nres, npos;
	lua_getglobal(L,func);

//	out2fmt_flush("func: %s\n", func);

	//push args
	for (narg=0; *sig; narg++ ) {
		npos=3 + narg; // 3 -$0, func, sig
		luaL_checkstack(L, 1, "too many arguments");
		switch (*sig++) {
			case 'd': /* double */
//				lua_pushnumber(L, va_arg(vl,double));
				break;
			case 'i': /* integer */
				lua_pushinteger(L, atoi(argv[npos]));
				break;
			case 's': /* string */
				lua_pushstring(L, argv[npos]);
				break;
			case '>': /* end of argument */
				goto endargs;
			default:
				cbsdlua_error(L,"invalid option (%c)", *(sig - 1 ));
		}
	}

	endargs:

	nres=strlen(sig);
	if (lua_pcall(L, narg, nres, 0)!= 0)
		cbsdlua_error(L, "error calling '%s': %s",func, lua_tostring(L,-1));

	//get result
	nres = -nres;
	while (*sig) {
		switch (*sig++) {
			case 'd': {
				int isnum;
				double n = lua_tonumberx(L, nres, &isnum);
				if (!isnum)
					cbsdlua_error(L,"wrong result type");
//				*va_arg(vl, double *) = n;
				break;
			}
			case 'i': {
				int isnum;
				int n = lua_tointegerx(L, nres, &isnum);
				if (!isnum)
					cbsdlua_error(L,"wrong result type");
//				*va_arg(vl, int *) = n;
//				out2fmt_flush("LUA Return for %s: %d\n", argv[npos], n);
				char str[100];
				fmtstr(str, sizeof(str), "%d", n);
				setvarsafe(argv[npos],str, 0);
				break;
			}
			case 's': {
				const char *s = lua_tostring(L, nres);
				if (s == NULL)
					cbsdlua_error(L,"wrong result type");
//				*va_arg(vl, const char **) = s;
//				out2fmt_flush("LUA Return for %s: %s\n", argv[npos], s);
				setvarsafe(argv[npos],s, 0);
				break;
			}
			default:
				cbsdlua_error(L,"invalid option (%c)", *(sig - 1 ));
		}
	nres++;
	}

	return 0;
}


// вызываем функцию на lua
// указываем тип входящих аргументов через d(oble),i(nteger),s(tring)
// символ > отделяет аргументы входащих от результата
// cbsdlua_func "mysum" "ii>i" "$w" "$h"
//int cbsdlua_funccmd(int argc, char **argv) {

//	va_list vl;
//	int narg, nres;

//	int w;
//	int h;
//	int z=0;

//	char *func = malloc(strlen(argv[1]));
//	memset(func, 0, strlen(argv[1]));
//	strcpy(func, argv[1]);

//	char *sig = malloc(strlen(argv[2]));
//	memset(sig, 0, strlen(argv[2]));
//	strcpy(sig, argv[2]);

//	call_va(L,"mysum","dd>d", w, h, &z);

//	return 0;
//}


void lua_loadscript (lua_State *L, const char *fname) {
	if ( luaL_loadfile(L, fname) || lua_pcall(L,0,0,0) )
		cbsdlua_error(L, "cannot load config. file: %s", lua_tostring(L, -1));
}

int cbsdlua_loadcmd(int argc, char **argv) {
	int error;
//	lua_State *L = luaL_newstate();
//	luaL_openlibs(L);

	if (argc!=2) {
		out1fmt("Use: cbsdlua_load <path>\n");
		return 1;
	}

	lua_loadscript(L,argv[1]);

//	lua_close(L);

	return 0;
}

//int cbsdlua_funccmd(int argc, char **argv) {
//	int w;
//	int h;
//	int z=0;
//
//	call_va(L,"mysum","ii>i", 2, 10, &z);
//
//	out2fmt_flush("%d", z);
//}



int cbsdluacmd(void) {
	char buff[256];
	int error;
//	lua_State *L = luaL_newstate();
	luaL_openlibs(L);

	while (fgets(buff,sizeof(buff), stdin) != NULL ) {
		error = luaL_loadstring(L,buff) || lua_pcall(L,0,0,0);

		if (error) {
			out2fmt_flush("%s\n", lua_tostring(L, -1 ));
			lua_pop(L,1);
		}
	}

//	lua_close(L);

	return 0;
}


