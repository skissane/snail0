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
	if (strcmp(cmdName,"selector.base") == 0) {
		unsigned long addr = 0;
		int rc = __dpmi_get_segment_base_address(block->selector,&addr);
		if (rc != 0) {
			*resultOut = snailDupString("__dpmi_get_segment_base_address call failed");
			return false;
		}
		*resultOut = snailI64ToStr(addr);
		return true;
	}
	if (strcmp(cmdName,"selector.limit") == 0) {
		unsigned limit = __dpmi_get_segment_limit(block->selector);
		*resultOut = snailI64ToStr(limit);
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
		uint8_t byte = 0;
		if (block->manuallyAllocated)
			byte = _farpeekb(block->selector, off);
		else
			dosmemget((block->segment << 4) + off, 1, &byte);
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
		uint8_t bdata = data;
		if (block->manuallyAllocated)
			_farpokeb(block->selector, off, bdata);
		else
			dosmemput(&bdata, 1, (block->segment << 4) + off);
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
	if (block->manuallyAllocated) {
		bool rc = __dpmi_free_ldt_descriptor(block->selector) == 0;
		free(block);
		if (!rc)
			return snailDupString("__dpmi_free_ldt_descriptor call failed");
	} else {
		bool rc = __dpmi_free_dos_memory(block->selector) == 0;
		free(block);
		if (!rc)
			return snailDupString("__dpmi_free_dos_memory call failed");
	}
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
	memset(buf, 0, len);
	snailDosMemoryBlock *block = channel->driverInfo;
	int32_t blockSize = block->paragraphs << 4;
	if (len > blockSize)
		len = blockSize;
	if (block->manuallyAllocated)
		movedata(block->selector, 0, _my_ds(), (uint32_t)buf, len);
	else
		dosmemget(block->segment << 4, len, buf);
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
	int32_t blockSize = block->paragraphs << 4;
	if (len > blockSize)
		len = blockSize;
	if (block->manuallyAllocated)
		movedata(_my_ds(), (uint32_t)buf, block->selector, 0, len);
	else
		dosmemput(buf, len, block->segment << 4);
	*written = len;
	return NULL;
}

#define SNAIL_DOS_INT_SETREG(_group,_name) \
	do { \
		if (strcmp(name,#_name) == 0) { \
			regs->_group._name = value; \
			return true; \
		} \
	} while (0)

bool snailDosSetReg(__dpmi_regs *regs, char *name, int32_t value) {
// Do we plan to call any 32-bit services?
// Are there any we could call?
// Comment this out until we need it (if we ever do).
// 	SNAIL_DOS_INT_SETREG(d,edi);
// 	SNAIL_DOS_INT_SETREG(d,esi);
// 	SNAIL_DOS_INT_SETREG(d,ebp);
// 	SNAIL_DOS_INT_SETREG(d,res);
// 	SNAIL_DOS_INT_SETREG(d,ebx);
// 	SNAIL_DOS_INT_SETREG(d,edx);
// 	SNAIL_DOS_INT_SETREG(d,ecx);
// 	SNAIL_DOS_INT_SETREG(d,eax);
	SNAIL_DOS_INT_SETREG(x,di);
	SNAIL_DOS_INT_SETREG(x,si);
	SNAIL_DOS_INT_SETREG(x,bp);
	SNAIL_DOS_INT_SETREG(x,bx);
	SNAIL_DOS_INT_SETREG(x,dx);
	SNAIL_DOS_INT_SETREG(x,cx);
	SNAIL_DOS_INT_SETREG(x,ax);
	SNAIL_DOS_INT_SETREG(x,flags);
	SNAIL_DOS_INT_SETREG(x,es);
	SNAIL_DOS_INT_SETREG(x,ds);
	SNAIL_DOS_INT_SETREG(x,fs);
	SNAIL_DOS_INT_SETREG(x,gs);
	SNAIL_DOS_INT_SETREG(x,ip);
	SNAIL_DOS_INT_SETREG(x,cs);
	SNAIL_DOS_INT_SETREG(x,sp);
	SNAIL_DOS_INT_SETREG(x,ss);
	SNAIL_DOS_INT_SETREG(h,bl);
	SNAIL_DOS_INT_SETREG(h,bh);
	SNAIL_DOS_INT_SETREG(h,dl);
	SNAIL_DOS_INT_SETREG(h,dh);
	SNAIL_DOS_INT_SETREG(h,cl);
	SNAIL_DOS_INT_SETREG(h,ch);
	SNAIL_DOS_INT_SETREG(h,al);
	SNAIL_DOS_INT_SETREG(h,ah);
	return false;
}

#undef SNAIL_DOS_INT_SETREG

#define SNAIL_DOS_INT_GETREG(_group,_name) \
	snailHashTablePut(out, #_name, snailI64ToStr(regs->_group._name))

void snailDosGetReg(__dpmi_regs *regs, snailHashTable *out) {
//	See above as to why 32-bit registers are commented-out.
// 	SNAIL_DOS_INT_GETREG(d,edi);
// 	SNAIL_DOS_INT_GETREG(d,esi);
// 	SNAIL_DOS_INT_GETREG(d,ebp);
// 	SNAIL_DOS_INT_GETREG(d,res);
// 	SNAIL_DOS_INT_GETREG(d,ebx);
// 	SNAIL_DOS_INT_GETREG(d,edx);
// 	SNAIL_DOS_INT_GETREG(d,ecx);
// 	SNAIL_DOS_INT_GETREG(d,eax);
	SNAIL_DOS_INT_GETREG(x,di);
	SNAIL_DOS_INT_GETREG(x,si);
	SNAIL_DOS_INT_GETREG(x,bp);
	SNAIL_DOS_INT_GETREG(x,bx);
	SNAIL_DOS_INT_GETREG(x,dx);
	SNAIL_DOS_INT_GETREG(x,cx);
	SNAIL_DOS_INT_GETREG(x,ax);
	SNAIL_DOS_INT_GETREG(x,flags);
	SNAIL_DOS_INT_GETREG(x,es);
	SNAIL_DOS_INT_GETREG(x,ds);
	SNAIL_DOS_INT_GETREG(x,fs);
	SNAIL_DOS_INT_GETREG(x,gs);
	SNAIL_DOS_INT_GETREG(x,ip);
	SNAIL_DOS_INT_GETREG(x,cs);
	SNAIL_DOS_INT_GETREG(x,sp);
	SNAIL_DOS_INT_GETREG(x,ss);
	SNAIL_DOS_INT_GETREG(h,bl);
	SNAIL_DOS_INT_GETREG(h,bh);
	SNAIL_DOS_INT_GETREG(h,dl);
	SNAIL_DOS_INT_GETREG(h,dh);
	SNAIL_DOS_INT_GETREG(h,cl);
	SNAIL_DOS_INT_GETREG(h,ch);
	SNAIL_DOS_INT_GETREG(h,al);
	SNAIL_DOS_INT_GETREG(h,ah);
}

#undef SNAIL_DOS_INT_GETREG
