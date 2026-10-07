// SPDX-License-Identifier: GPL-2.0-only
/*
 * Pass a signed Amlogic video microcode package to the vendor video
 * firmware trusted application, which verifies, decrypts and keeps it in
 * secure memory. The decoder driver then asks the secure monitor to load
 * the image for each codec into the AMRISC.
 *
 * Copyright (C) 2026-present Team LibreELEC (https://libreelec.tv)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tee_client_api.h>

#define CHUNK_SIZE	0x20000
#define CMD_LOAD	0
#define SIG_SIZE	256

static const TEEC_UUID video_fw_uuid = {
	0x526fc4fc, 0x7ee6, 0x4a12,
	{ 0x96, 0xe3, 0x83, 0xda, 0x95, 0x65, 0xbc, 0xe8 }
};

static unsigned char *read_file(const char *path, long *size)
{
	unsigned char *buf;
	FILE *f;

	f = fopen(path, "rb");
	if (!f)
		return NULL;

	if (fseek(f, 0, SEEK_END) || (*size = ftell(f)) <= 0 ||
	    fseek(f, 0, SEEK_SET)) {
		fclose(f);
		return NULL;
	}

	buf = malloc(*size);
	if (buf && fread(buf, 1, *size, f) != (size_t)*size) {
		free(buf);
		buf = NULL;
	}
	fclose(f);

	return buf;
}

int main(int argc, char **argv)
{
	TEEC_SharedMemory shm = { .size = CHUNK_SIZE, .flags = TEEC_MEM_INPUT };
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_Result res;
	unsigned char *fw;
	uint32_t origin;
	long size, sent = 0;
	int ret = 1;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <video_ucode.bin>\n", argv[0]);
		return 2;
	}

	fw = read_file(argv[1], &size);
	if (!fw) {
		fprintf(stderr, "cannot read %s\n", argv[1]);
		return 1;
	}

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_InitializeContext: 0x%x\n", res);
		goto free_fw;
	}

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
					 TEEC_NONE, TEEC_NONE);
	op.params[0].value.a = size;
	op.params[1].value.a = SIG_SIZE;
	res = TEEC_OpenSession(&ctx, &sess, &video_fw_uuid, TEEC_LOGIN_PUBLIC,
			       NULL, &op, &origin);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_OpenSession: 0x%x origin %u\n", res, origin);
		goto finalize;
	}

	res = TEEC_AllocateSharedMemory(&ctx, &shm);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_AllocateSharedMemory: 0x%x\n", res);
		goto close;
	}

	while (sent < size) {
		size_t len = size - sent > CHUNK_SIZE ? CHUNK_SIZE : size - sent;

		memcpy(shm.buffer, fw + sent, len);
		memset(&op, 0, sizeof(op));
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT,
						 TEEC_NONE, TEEC_NONE, TEEC_NONE);
		op.params[0].memref.parent = &shm;
		op.params[0].memref.size = len;

		res = TEEC_InvokeCommand(&sess, CMD_LOAD, &op, &origin);
		if (res != TEEC_SUCCESS && res != TEEC_ERROR_BUSY) {
			fprintf(stderr, "TEEC_InvokeCommand at %ld: 0x%x origin %u\n",
				sent, res, origin);
			goto release;
		}
		sent += len;
	}

	printf("loaded %s (%ld bytes)\n", argv[1], size);
	ret = 0;

release:
	TEEC_ReleaseSharedMemory(&shm);
close:
	TEEC_CloseSession(&sess);
finalize:
	TEEC_FinalizeContext(&ctx);
free_fw:
	free(fw);
	return ret;
}
