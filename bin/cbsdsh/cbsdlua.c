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
void call_va(lua_State *L, const char *func, const char *sig, ...) {
	va_list vl;
	int narg, nres;
	va_start(vl,sig);
	lua_getglobal(L,func);

	//push args
	for (narg=0; *sig; narg++ ) {
		luaL_checkstack(L, 1, "too many arguments");
		switch (*sig++) {
			case 'd': /* double */
				lua_pushnumber(L, va_arg(vl,double));
				break;
			case 'i': /* integer */
				lua_pushinteger(L, va_arg(vl,int));
				break;
			case 's': /* string */
				lua_pushstring(L, va_arg(vl,char *));
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
				*va_arg(vl, double *) = n;
				break;
			}
			case 'i': {
				int isnum;
				int n = lua_tointegerx(L, nres, &isnum);
				if (!isnum)
					cbsdlua_error(L,"wrong result type");
				*va_arg(vl, int *) = n;
				break;
			}
			case 's': {
				const char *s = lua_tostring(L, nres);
				if (s == NULL)
					cbsdlua_error(L,"wrong result type");
				*va_arg(vl, const char **) = s;
				break;
			}
			default:
				cbsdlua_error(L,"invalid option (%c)", *(sig - 1 ));
		}
	nres++;
	}

	va_end(vl);

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


int cbsdlua_funccmd(int argc,char **argv) {

	int w=0;
	int h=0;

	if (argc>2) {
		w=atoi(argv[3]);
		h=atoi(argv[4]);
	}

	int z=0;

//	out2fmt_flush("KK=%s\n", argv[3]);
//	out2fmt_flush("KK=%d\n", argc);

	// only exec func without args
	if (argc==2)
		call_va(L,argv[1],">");
	else
		call_va(L,argv[1],argv[2], w, h, &z);

//	call_va(L,argv[1],argv[2]);

	return 0;
}








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
