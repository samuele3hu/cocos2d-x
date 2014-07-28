
#include "tolua_fix.h"
#include "base/CCRef.h"
#include "LuaBasicConversions.h"
#include <stdlib.h>

using namespace cocos2d;

static int s_function_ref_id = 0;

TOLUA_API void toluafix_open(lua_State* L)
{
    lua_pushstring(L, TOLUA_REFID_PTR_MAPPING);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);

    lua_pushstring(L, TOLUA_REFID_TYPE_MAPPING);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);

    lua_pushstring(L, TOLUA_REFID_FUNCTION_MAPPING);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);
}

TOLUA_API int toluafix_pushusertype_ccobject(lua_State* L,
                                             int refid,
                                             int* p_refid,
                                             void* ptr,
                                             const char* type)
{
    toluafix_pushusertype_ref(L, ptr, type);
    return 0;
    
//    if (ptr == NULL || p_refid == NULL)
//    {
//        lua_pushnil(L);
//        return -1;
//    }
//    
//    Ref* vPtr = static_cast<Ref*>(ptr);
//    const char* vType = getLuaTypeName(vPtr, type);
//
//    if (*p_refid == 0)
//    {
//        *p_refid = refid;
//
//        lua_pushstring(L, TOLUA_REFID_PTR_MAPPING);
//        lua_rawget(L, LUA_REGISTRYINDEX);                           /* stack: refid_ptr */
//        lua_pushinteger(L, refid);                                  /* stack: refid_ptr refid */
//        lua_pushlightuserdata(L, vPtr);                              /* stack: refid_ptr refid ptr */
//
//        lua_rawset(L, -3);                  /* refid_ptr[refid] = ptr, stack: refid_ptr */
//        lua_pop(L, 1);                                              /* stack: - */
//
//        lua_pushstring(L, TOLUA_REFID_TYPE_MAPPING);
//        lua_rawget(L, LUA_REGISTRYINDEX);                           /* stack: refid_type */
//        lua_pushinteger(L, refid);                                  /* stack: refid_type refid */
//        lua_pushstring(L, vType);                                    /* stack: refid_type refid type */
//        lua_rawset(L, -3);                /* refid_type[refid] = type, stack: refid_type */
//        lua_pop(L, 1);                                              /* stack: - */
//
//        //printf("[LUA] push CCObject OK - refid: %d, ptr: %x, type: %s\n", *p_refid, (int)ptr, type);
//    }
//
//    tolua_pushusertype_and_addtoroot(L, vPtr, vType);
    
    return 0;
}

TOLUA_API int toluafix_remove_ccobject_by_refid(lua_State* L, int refid)
{
	void* ptr = NULL;
    const char* type = NULL;
    void** ud = NULL;
    if (refid == 0) return -1;

    // get ptr from tolua_refid_ptr_mapping
    lua_pushstring(L, TOLUA_REFID_PTR_MAPPING);
    lua_rawget(L, LUA_REGISTRYINDEX);                               /* stack: refid_ptr */
    lua_pushinteger(L, refid);                                      /* stack: refid_ptr refid */
    lua_rawget(L, -2);                                              /* stack: refid_ptr ptr */
    ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);                                                  /* stack: refid_ptr */
    if (ptr == NULL)
    {
        lua_pop(L, 1);
        // Lua stack has closed, C++ object not in Lua.
        // printf("[LUA ERROR] remove CCObject with NULL ptr, refid: %d\n", refid);
        return -2;
    }

    // remove ptr from tolua_refid_ptr_mapping
    lua_pushinteger(L, refid);                                      /* stack: refid_ptr refid */
    lua_pushnil(L);                                                 /* stack: refid_ptr refid nil */
    lua_rawset(L, -3);                     /* delete refid_ptr[refid], stack: refid_ptr */
    lua_pop(L, 1);                                                  /* stack: - */


    // get type from tolua_refid_type_mapping
    lua_pushstring(L, TOLUA_REFID_TYPE_MAPPING);
    lua_rawget(L, LUA_REGISTRYINDEX);                               /* stack: refid_type */
    lua_pushinteger(L, refid);                                      /* stack: refid_type refid */
    lua_rawget(L, -2);                                              /* stack: refid_type type */
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 2);
        printf("[LUA ERROR] remove CCObject with NULL type, refid: %d, ptr: %p\n", refid, ptr);
        return -1;
    }

    type = lua_tostring(L, -1);
    lua_pop(L, 1);                                                  /* stack: refid_type */

    // remove type from tolua_refid_type_mapping
    lua_pushinteger(L, refid);                                      /* stack: refid_type refid */
    lua_pushnil(L);                                                 /* stack: refid_type refid nil */
    lua_rawset(L, -3);                    /* delete refid_type[refid], stack: refid_type */
    lua_pop(L, 1);                                                  /* stack: - */

    // get ubox
    luaL_getmetatable(L, type);                                     /* stack: mt */
    lua_pushstring(L, "tolua_ubox");                                /* stack: mt key */
    lua_rawget(L, -2);                                              /* stack: mt ubox */
    if (lua_isnil(L, -1))
    {
        // use global ubox
        lua_pop(L, 1);                                              /* stack: mt */
        lua_pushstring(L, "tolua_ubox");                            /* stack: mt key */
        lua_rawget(L, LUA_REGISTRYINDEX);                           /* stack: mt ubox */
    };
    
    
    // cleanup root
    tolua_remove_value_from_root(L, ptr);

    lua_pushlightuserdata(L, ptr);                                  /* stack: mt ubox ptr */
    lua_rawget(L,-2);                                               /* stack: mt ubox ud */
    if (lua_isnil(L, -1))
    {
        // Lua object has released (GC), C++ object not in ubox.
        //printf("[LUA ERROR] remove CCObject with NULL ubox, refid: %d, ptr: %x, type: %s\n", refid, (int)ptr, type);
        lua_pop(L, 3);
        return -3;
    }

    // cleanup peertable
    lua_pushvalue(L, LUA_REGISTRYINDEX);
    lua_setfenv(L, -2);

    ud = (void**)lua_touserdata(L, -1);
    lua_pop(L, 1);                                                  /* stack: mt ubox */
    if (ud == NULL)
    {
        printf("[LUA ERROR] remove CCObject with NULL userdata, refid: %d, ptr: %p, type: %s\n", refid, ptr, type);
        lua_pop(L, 2);
        return -1;
    }

    // clean userdata
    *ud = NULL;

    lua_pushlightuserdata(L, ptr);                                  /* stack: mt ubox ptr */
    lua_pushnil(L);                                                 /* stack: mt ubox ptr nil */
    lua_rawset(L, -3);                             /* ubox[ptr] = nil, stack: mt ubox */

    lua_pop(L, 2);
    //printf("[LUA] remove CCObject, refid: %d, ptr: %x, type: %s\n", refid, (int)ptr, type);
    return 0;
}

TOLUA_API int toluafix_ref_function(lua_State* L, int lo, int def)
{
    // function at lo
    if (!lua_isfunction(L, lo)) return 0;

    s_function_ref_id++;

    lua_pushstring(L, TOLUA_REFID_FUNCTION_MAPPING);
    lua_rawget(L, LUA_REGISTRYINDEX);                           /* stack: fun ... refid_fun */
    lua_pushinteger(L, s_function_ref_id);                      /* stack: fun ... refid_fun refid */
    lua_pushvalue(L, lo);                                       /* stack: fun ... refid_fun refid fun */

    lua_rawset(L, -3);                  /* refid_fun[refid] = fun, stack: fun ... refid_ptr */
    lua_pop(L, 1);                                              /* stack: fun ... */

    return s_function_ref_id;

    // lua_pushvalue(L, lo);                                           /* stack: ... func */
    // return luaL_ref(L, LUA_REGISTRYINDEX);
}

TOLUA_API void toluafix_get_function_by_refid(lua_State* L, int refid)
{
    lua_pushstring(L, TOLUA_REFID_FUNCTION_MAPPING);
    lua_rawget(L, LUA_REGISTRYINDEX);                           /* stack: ... refid_fun */
    lua_pushinteger(L, refid);                                  /* stack: ... refid_fun refid */
    lua_rawget(L, -2);                                          /* stack: ... refid_fun fun */
    lua_remove(L, -2);                                          /* stack: ... fun */
}

TOLUA_API void toluafix_remove_function_by_refid(lua_State* L, int refid)
{
    lua_pushstring(L, TOLUA_REFID_FUNCTION_MAPPING);
    lua_rawget(L, LUA_REGISTRYINDEX);                           /* stack: ... refid_fun */
    lua_pushinteger(L, refid);                                  /* stack: ... refid_fun refid */
    lua_pushnil(L);                                             /* stack: ... refid_fun refid nil */
    lua_rawset(L, -3);                  /* refid_fun[refid] = fun, stack: ... refid_ptr */
    lua_pop(L, 1);                                              /* stack: ... */

    // luaL_unref(L, LUA_REGISTRYINDEX, refid);
}

// check lua value is funciton
TOLUA_API int toluafix_isfunction(lua_State* L, int lo, const char* type, int def, tolua_Error* err)
{
    if (lua_gettop(L) >= abs(lo) && lua_isfunction(L, lo))
    {
        return 1;
    }
    err->index = lo;
    err->array = 0;
    err->type = "[not function]";
    return 0;
}

TOLUA_API int toluafix_totable(lua_State* L, int lo, int def)
{
    return lo;
}

TOLUA_API int toluafix_istable(lua_State* L, int lo, const char* type, int def, tolua_Error* err)
{
    return tolua_istable(L, lo, def, err);
}

TOLUA_API void toluafix_stack_dump(lua_State* L, const char* label)
{
    int i;
    int top = lua_gettop(L);
    printf("Total [%d] in lua stack: %s\n", top, label != 0 ? label : "");
    for (i = -1; i >= -top; i--)
    {
        int t = lua_type(L, i);
        switch (t)
        {
            case LUA_TSTRING:
                printf("  [%02d] string %s\n", i, lua_tostring(L, i));
                break;
            case LUA_TBOOLEAN:
                printf("  [%02d] boolean %s\n", i, lua_toboolean(L, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:
                printf("  [%02d] number %g\n", i, lua_tonumber(L, i));
                break;
            default:
                printf("  [%02d] %s\n", i, lua_typename(L, t));
        }
    }
    printf("\n");
}

void toluafix_pushusertype_ref(lua_State* L, void* ptr, const char* type)
{
    if (nullptr == ptr || nullptr == type)
    {
        lua_pushnil(L);
    }
    else
    {
        cocos2d::Ref* vPtr = static_cast<Ref*>(ptr);
        const char* vType  = getLuaTypeName(vPtr, type);
        luaL_getmetatable(L, vType);      /* stack: mt */
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);                /* NOT FOUND metatable,stack:*/
            lua_pushnil(L);
            return;
        }
        
        lua_pushstring(L, "tolua_ubox");  /* stack: mt string */
        lua_rawget(L, -2);                /* stack: mt ubox */
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            lua_pushstring(L, "tolua_ubox");
            lua_rawget(L, LUA_REGISTRYINDEX);
        }

        lua_pushlightuserdata(L,vPtr);   /* stack: mt ubox key<value> */
        lua_rawget(L,-2);                /* stack: mt ubox ubox[value] */
        
        if (lua_isnil(L,-1))
        {
            lua_pop(L,1);                                           /* stack: mt ubox */
            lua_pushlightuserdata(L,vPtr);
            *(void**)lua_newuserdata(L,sizeof(void *)) = vPtr;     /* stack: mt ubox value newud */
            lua_pushvalue(L,-1);                                    /* stack: mt ubox value newud newud */
            lua_insert(L,-4);                                       /* stack: mt newud ubox value newud */
            lua_rawset(L,-3);                  /* ubox[value] = newud, stack: mt newud ubox */
            lua_pop(L,1);                                           /* stack: mt newud */
            /*luaL_getmetatable(L,type);*/
            lua_pushvalue(L, -2);                                   /* stack: mt newud mt */
            lua_setmetatable(L,-2);                      /* update mt, stack: mt newud */
            
#ifdef LUA_VERSION_NUM
            lua_pushvalue(L, TOLUA_NOPEER);             /* stack: mt newud peer */
            lua_setfenv(L, -2);                         /* stack: mt newud */
#endif
            lua_remove(L, -2);                       /* stack: ubox[u]*/
            vPtr->retain();
            cocos2d::log("referencount is %d", vPtr->_referenceCount);
            tolua_register_gc(L,lua_gettop(L));
        }
        else
        {
            /* check the need of updating the metatable to a more specialized class */
            lua_insert(L,-2);                                       /* stack: mt ubox[u] ubox */
            lua_pop(L,1);                                           /* stack: mt ubox[u] */
            lua_pushstring(L,"tolua_super");
            lua_rawget(L,LUA_REGISTRYINDEX);                        /* stack: mt ubox[u] super */
            lua_getmetatable(L,-2);                                 /* stack: mt ubox[u] super mt */
            lua_rawget(L,-2);                                       /* stack: mt ubox[u] super super[mt] */
            if (lua_istable(L,-1))
            {
                lua_pushstring(L,vType);                             /* stack: mt ubox[u] super super[mt] type */
                lua_rawget(L,-2);                                   /* stack: mt ubox[u] super super[mt] flag */
                if (lua_toboolean(L,-1) == 1)                       /* if true */
                {
                    lua_pop(L,3);                                   /* mt ubox[u]*/
                    lua_remove(L, -2);
                    return;
                }
            }
            /* type represents a more specilized type */
            /*luaL_getmetatable(L,type);             // stack: mt ubox[u] super super[mt] flag mt */
            lua_pushvalue(L, -5);                    /* stack: mt ubox[u] super super[mt] flag mt */
            lua_setmetatable(L,-5);                  /* stack: mt ubox[u] super super[mt] flag */
            lua_pop(L,3);                            /* stack: mt ubox[u] */
            lua_remove(L, -2);                       /* stack: ubox[u]*/
        }
        
    }
}

void toluafix_pushusertype_no_ref(lua_State* L, void* ptr, const char* type)
{
    if (nullptr == ptr || nullptr == type)
    {
        lua_pushnil(L);
    }
    else
    {
        luaL_getmetatable(L, type);      /* stack: mt */
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);                /* NOT FOUND metatable,stack:*/
            lua_pushnil(L);
            return;
        }
        
        lua_pushstring(L, "tolua_ubox");  /* stack: mt string */
        lua_rawget(L, -2);                /* stack: mt ubox */
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            lua_pushstring(L, "tolua_ubox");
            lua_rawget(L, LUA_REGISTRYINDEX);
        }
        //TODO CAREFUL
        lua_pushlightuserdata(L,ptr);   /* stack: mt ubox key<value> */
        lua_rawget(L,-2);                /* stack: mt ubox ubox[value] */
        
        if (lua_isnil(L,-1))
        {
            lua_pop(L,1);                                           /* stack: mt ubox */
            lua_pushlightuserdata(L,ptr);
            *(void**)lua_newuserdata(L,sizeof(void *)) = ptr;     /* stack: mt ubox value newud */
            lua_pushvalue(L,-1);                                    /* stack: mt ubox value newud newud */
            lua_insert(L,-4);                                       /* stack: mt newud ubox value newud */
            lua_rawset(L,-3);                  /* ubox[value] = newud, stack: mt newud ubox */
            lua_pop(L,1);                                           /* stack: mt newud */
            /*luaL_getmetatable(L,type);*/
            lua_pushvalue(L, -2);                                   /* stack: mt newud mt */
            lua_setmetatable(L,-2);                      /* update mt, stack: mt newud */
            
#ifdef LUA_VERSION_NUM
            lua_pushvalue(L, TOLUA_NOPEER);             /* stack: mt newud peer */
            lua_setfenv(L, -2);                         /* stack: mt newud */
#endif
            lua_remove(L, -2);    /* stack: ubox[u]*/
            //tolua_register_gc(L,lua_gettop(L));
        }
        else
        {
            /* check the need of updating the metatable to a more specialized class */
            lua_insert(L,-2);                                       /* stack: mt ubox[u] ubox */
            lua_pop(L,1);                                           /* stack: mt ubox[u] */
            lua_pushstring(L,"tolua_super");
            lua_rawget(L,LUA_REGISTRYINDEX);                        /* stack: mt ubox[u] super */
            lua_getmetatable(L,-2);                                 /* stack: mt ubox[u] super mt */
            lua_rawget(L,-2);                                       /* stack: mt ubox[u] super super[mt] */
            if (lua_istable(L,-1))
            {
                lua_pushstring(L,type);                             /* stack: mt ubox[u] super super[mt] type */
                lua_rawget(L,-2);                                   /* stack: mt ubox[u] super super[mt] flag */
                if (lua_toboolean(L,-1) == 1)                       /* if true */
                {
                    lua_pop(L,3);                                   /* mt ubox[u]*/
                    lua_remove(L, -2);
                    return;
                }
            }
            /* type represents a more specilized type */
            /*luaL_getmetatable(L,type);             // stack: mt ubox[u] super super[mt] flag mt */
            lua_pushvalue(L, -5);                    /* stack: mt ubox[u] super super[mt] flag mt */
            lua_setmetatable(L,-5);                /* stack: mt ubox[u] super super[mt] flag */
            lua_pop(L,3);                          /* stack: mt ubox[u] */
        }
    }
}
