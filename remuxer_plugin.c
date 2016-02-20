#include <uwsgi.h>
#include "remuxlib.h"

static int remuxer_request(struct wsgi_request *wsgi_req) {

	if (uwsgi_parse_vars(wsgi_req)) {
		return -1;
	}

	if (uwsgi_response_prepare_headers(wsgi_req, "200 OK", 6)) return -1;
	if (uwsgi_response_add_content_type(wsgi_req, "text/plain", 10)) return -1;
	//uwsgi_log("remuxer plugin step 1\n");
	
	uint16_t vlen1 = 0;
	uint16_t vlen2 = 0;
	
	char *in = uwsgi_get_var(wsgi_req, "IN", 2, &vlen1);
	char *out = uwsgi_get_var(wsgi_req, "OUT", 3, &vlen2);
	
	char in_fname[255];
	char out_fname[255];
	
	memcpy(in_fname, in, vlen1);
	in_fname[vlen1] = '\0';

	memcpy(out_fname, out, vlen2);
	out_fname[vlen2] = '\0';

	//uwsgi_log("IN:%s OUT: %s \n", in_fname, out_fname);

	//remux(in_fname, out_fname);
	remux_cmd("/Users/os/ffmpeg_remux.sh", in_fname, out_fname);
	
	return UWSGI_OK;
}

struct uwsgi_plugin remuxer_plugin = {
	.name = "remuxer",
	.modifier1 = 0,
	.request = remuxer_request,
};
