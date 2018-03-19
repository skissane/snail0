/***---DOS: FUNCTIONS---***/
void snailChannelSetup_DOSMEM(snailInterp *snail) {
	snailChannelDriver * DOSMEM = snailMalloc(sizeof(snailChannelDriver));
	DOSMEM->name = snailDupString("DOSMEM");
	DOSMEM->f_OPEN = snailChannel_OPEN_dosmem;
	DOSMEM->f_CLOSE = snailChannel_CLOSE_dosmem;
	DOSMEM->f_CONTROL = snailChannel_CONTROL_dosmem;
	DOSMEM->f_READ = snailChannel_READ_dosmem;
	DOSMEM->f_WRITE = snailChannel_WRITE_dosmem;
	char *msg = snailChannelDriverRegister(snail,DOSMEM);
	if (msg != NULL)
		snailPanic(msg);
}

bool snailChannel_CONTROL_dosmem(snailChannel *channel, snailArray *cmdIn, char **resultOut) {
	if (cmdIn->length == 0) {
		*resultOut = snailDupString("DOSMEM: invalid CONTROL command");
		return false;
	}
	char *cmdName = cmdIn->elems[0];
	snailDosMemoryBlock *block = channel->driverInfo;
	if (strcmp(cmdName,"size") == 0) {
		*resultOut = snailI64ToStr(block->paragraphs << 4);
		return true;
	}
	if (strcmp(cmdName,"segment") == 0) {
		*resultOut = snailI64ToStr(block->segment);
		return true;
	}
	if (strcmp(cmdName,"selector") == 0) {
		*resultOut = snailI64ToStr(block->selector);
		return true;
	}
	if (strcmp(cmdName,"byte.get") == 0) {
		if (cmdIn->length != 2) {
			*resultOut = snailDupString("DOSMEM: bad argument count for 'byte.get' control command");
			return false;
		}
		if (!snailIsInt(cmdIn->elems[1])) {
			*resultOut = snailDupString("DOSMEM: 'byte.get' argument must be integer");
			return false;
		}
		int32_t off = strtol(cmdIn->elems[1],NULL,10);
		if (off < 0 || off >= (block->paragraphs << 4)) {
			*resultOut = snailDupString("DOSMEM: 'byte.get' argument out of valid range");
			return false;
		}
		uint8_t *addr = (uint8_t*)(block->segment << 4);
		uint8_t byte = addr[off];
		*resultOut = snailI64ToStr(byte);
		return true;
	}
	if (strcmp(cmdName,"byte.set") == 0) {
		if (cmdIn->length != 3) {
			*resultOut = snailDupString("DOSMEM: bad argument count for 'byte.set' control command");
			return false;
		}
		if (!snailIsInt(cmdIn->elems[1])) {
			*resultOut = snailDupString("DOSMEM: 'byte.set' 1st argument must be integer");
			return false;
		}
		if (!snailIsInt(cmdIn->elems[2])) {
			*resultOut = snailDupString("DOSMEM: 'byte.set' 2nd argument must be integer");
			return false;
		}
		int32_t off = strtol(cmdIn->elems[1],NULL,10);
		if (off < 0 || off >= (block->paragraphs << 4)) {
			*resultOut = snailDupString("DOSMEM: 'byte.set' 1st argument out of valid range");
			return false;
		}
		int32_t data = strtol(cmdIn->elems[2],NULL,10);
		if (data < 0 || data > 255) {
			*resultOut = snailDupString("DOSMEM: 'byte.set' 2nd argument out of valid range");
			return false;
		}
		uint8_t *addr = (uint8_t*)(block->segment << 4);
		addr[off] = data;
		*resultOut = NULL;
		return true;
	}
	snailBuffer *msg = snailBufferCreate(16);
	snailBufferAddString(msg,"DOSMEM: unsupported control command: '");
	snailBufferAddString(msg,cmdIn->elems[0]);
	snailBufferAddString(msg,"'");
	snailBufferAddChar(msg,0);
	*resultOut = snailDupString(msg->bytes);
	snailBufferDestroy(msg);
	return false;
}

char *snailChannel_CLOSE_dosmem(snailChannel *channel) {
	snailDosMemoryBlock *block = channel->driverInfo;
	bool rc = __dpmi_free_dos_memory(block->selector) == 0;
	free(block);
	if (!rc)
		return snailDupString("__dpmi_free_dos_memory call failed");
	return NULL;
}

char *snailChannel_OPEN_dosmem(snailChannel *channel, void *driverArg) {
	if (driverArg == NULL)
		return snailDupString("argument is null");
	channel->driverInfo = driverArg;
	return NULL;
}

char *snailChannel_READ_dosmem(snailChannel *channel, void *buf, size_t len, size_t *read) {
	if (len == 0) {
		*read = 0;
		return NULL;
	}
	snailDosMemoryBlock *block = channel->driverInfo;
	void *addr = (void*)(block->segment << 4);
	int32_t blockSize = block->paragraphs << 4;
	if (len > blockSize - 1)
		len = blockSize - 1;
	memcpy(buf, addr, len);
	char *cbuf = buf;
	cbuf[len - 1] = 0;
	*read = len;
	return NULL;
}

char *snailChannel_WRITE_dosmem(snailChannel *channel, void *buf, size_t len, size_t *written) {
	if (len == 0) {
		*written = 0;
		return NULL;
	}
	snailDosMemoryBlock *block = channel->driverInfo;
	void *addr = (void *)(block->segment << 4);
	int32_t blockSize = block->paragraphs << 4;
	if (len > blockSize)
		len = blockSize;
	memcpy(addr, buf, len);
	*written = len;
	return NULL;
}
