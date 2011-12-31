#include <time.h>

#include <glib.h>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>

#include "nogaim.h"

MonoAssembly *assembly = NULL;
MonoDomain *domain = NULL;
MonoImage *image = NULL;
MonoClass *conn_class = NULL;
void (*steam_mono_send_message)(MonoObject*, MonoString*, MonoString*) = NULL;
void (*steam_mono_login)(MonoObject*, MonoString*, MonoString*, MonoString*) = NULL;
void (*steam_mono_logout)(MonoObject*) = NULL;

void steam_init(account_t *acc) {
	set_add(&acc->set, "steam_guard_code", NULL, NULL, acc);
}

void steam_login(account_t *acc) {
	struct im_connection *ic = imcb_new(acc);

	MonoObject *connection = mono_object_new(domain, conn_class);

	MonoMethodDesc *desc;
	desc = mono_method_desc_new(":.ctor", FALSE);
	MonoMethod *conn_ctor;
	conn_ctor = mono_method_desc_search_in_class(desc, conn_class);
	void *args[1] = { &ic };
	mono_runtime_invoke(conn_ctor, connection, args, NULL);

	ic->proto_data = connection;
	MonoString *username = mono_string_new(domain, acc->user);
	MonoString *password = mono_string_new(domain, acc->pass);
	MonoString *guard    = mono_string_new(domain, set_getstr(&acc->set, "steam_guard_code"));
	steam_mono_login(connection, username, password, guard);
}

void steam_logout(struct im_connection *ic) {
	MonoObject *connection = ic->proto_data;
	steam_mono_logout(connection);
}

int steam_buddy_msg(struct im_connection *ic, char *name, char *message, int flags) {
	MonoObject *connection = ic->proto_data;
	MonoString *mono_name = mono_string_new(domain, name);
	MonoString *mono_message = mono_string_new(domain, name);
	steam_mono_send_message(connection, mono_name, mono_message);
	return 0;
}

void steam_add_buddy(struct im_connection *ic, char *name, char *group) {
  // todo
}

void steam_remove_buddy(struct im_connection *ic, char *name, char *group) {
  // todo
}

void* get_method(char *name) {
	MonoMethodDesc *desc;
	MonoMethod *method;
        void *func;

	desc   = mono_method_desc_new(name, FALSE);
	method = mono_method_desc_search_in_class(desc, conn_class);
        func   = mono_method_get_unmanaged_thunk(method);
 
//	free(desc);
//	free(method);

	return func;
}

// wrapped due to time_t
void steam_receive_message(struct im_connection *ic, char *name, char *message) {
	imcb_buddy_msg(ic, name, message, 0, time(NULL));
}

void steam_initmodule() {
	domain = mono_jit_init("BitlSteam");
	assembly = mono_domain_assembly_open (domain, "BitlSteam.dll");

	image = mono_assembly_get_image(assembly);

	conn_class = mono_class_from_name(image, "BitlSteam", "SteamConnection");

	steam_mono_send_message = get_method(":SendMessage(String,String)");
	steam_mono_login = get_method(":Login(String,String,String)");
	steam_mono_logout = get_method(":Logout()");

//	free(image);
//	free(conn_class);

	struct prpl *ret = g_new0(struct prpl, 1);

	ret->options = OPT_NOOTR;
	ret->name = "steam";
	ret->login = steam_login;
	ret->init = steam_init;
	ret->logout = steam_logout;
	ret->buddy_msg = steam_buddy_msg;
	ret->add_buddy = steam_add_buddy;
	ret->remove_buddy = steam_remove_buddy;
	ret->handle_cmp = g_strcasecmp;

	register_protocol(ret);
}
