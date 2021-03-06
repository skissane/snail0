/***---DOS NATIVES: IMPLEMENT---***/
NATIVE(dos_mem_fixed,2) {
	NATIVE_ARG_MUSTINT(0);
	NATIVE_ARG_MUSTINT(1);
	int32_t bytes = strtol(args[0],NULL,10);
	if (bytes <= 0) {
		snailSetResult(snail, "dos.mem.fixed: argument cannot be zero or negative");
		return snailStatusError;
	}
	if (bytes > 65535) {
		snailSetResult(snail, "dos.mem.fixed: block size has 64KB maximum");
		return snailStatusError;
	}
	int32_t segment = strtol(args[1],NULL,10);
	if (segment < 0 || segment > 0xFFFF) {
		snailSetResult(snail, "dos.mem.fixed: requested segment out of valid range");
		return snailStatusError;
	}
	snailDosMemoryBlock *block = snailMalloc(sizeof(snailDosMemoryBlock));
	block->paragraphs = (bytes + 15) >> 4;
	block->segment = segment;
	block->selector = __dpmi_allocate_ldt_descriptors(1);
	block->manuallyAllocated = true;
	if (block->selector == -1) {
		free(block);
		snailSetResult(snail,"__dpmi_allocate_ldt_descriptors failed");
		return snailStatusError;
	}
	uint32_t linear = segment << 4;
	if (__dpmi_set_segment_base_address(block->selector, linear) != 0) {
		__dpmi_free_ldt_descriptor(block->selector);
		free(block);
		snailSetResult(snail,"__dpmi_set_segment_base_address failed");
		return snailStatusError;
	}
	if (__dpmi_set_segment_limit(block->selector, (block->paragraphs << 4)) != 0) {
		__dpmi_free_ldt_descriptor(block->selector);
		free(block);
		snailSetResult(snail,"__dpmi_set_segment_limit failed");
		return snailStatusError;
	}
	if (__dpmi_set_descriptor_access_rights(block->selector, 0x40f3) != 0) {
		__dpmi_free_ldt_descriptor(block->selector);
		free(block);
		snailSetResult(snail,"__dpmi_set_descriptor_access_rights failed");
		return snailStatusError;
	}
	char *channelName = snailChannelMakeName(snail);
	char *error = snailChannelRegister(snail, channelName, "DOSMEM", block);
	assert(error==NULL);
	snailSetResult(snail,channelName);
	free(channelName);
	return snailStatusOk;
}

NATIVE(dos_mem_alloc,1) {
	NATIVE_ARG_MUSTINT(0);

	int32_t bytes = strtol(args[0],NULL,10);
	if (bytes <= 0) {
		snailSetResult(snail, "dos.mem.alloc: argument cannot be zero or negative");
		return snailStatusError;
	}
	if (bytes > 65535) {
		snailSetResult(snail, "dos.mem.alloc: block size has 64KB maximum");
		return snailStatusError;
	}
	snailDosMemoryBlock *block = snailMalloc(sizeof(snailDosMemoryBlock));
	block->paragraphs = (bytes + 15) >> 4;
	block->segment = __dpmi_allocate_dos_memory(block->paragraphs, (int*) &(block->selector));
	if (block->segment == -1) {
		free(block);
		snailSetResult(snail,"__dpmi_allocate_dos_memory failed");
		return snailStatusError;
	}
	uint8_t zero = 0;
	for (size_t i = 0; i < (block->paragraphs << 4); i++)
		dosmemput(&zero, 1, (block->segment << 4) + i);
	char *channelName = snailChannelMakeName(snail);
	char *error = snailChannelRegister(snail, channelName, "DOSMEM", block);
	assert(error==NULL);
	snailSetResult(snail,channelName);
	free(channelName);
	return snailStatusOk;
}

NATIVE(dos_int86,2) {
	NATIVE_ARG_MUSTINT(0);
	NATIVE_ARG_MUSTCLASS(1, 'D');
	int32_t vector = strtol(args[0],NULL,10);
	if (vector < 0 || vector > 255) {
		snailSetResult(snail,"dos.int86: vector out of range");
		return snailStatusError;
	}
	snailHashTable *regs = snailParseDict(args[1]);
	snailArray *names = snailHashTableKeys(regs);
	__dpmi_regs r;
	memset(&r,0,sizeof(__dpmi_regs));
	for (int i = 0; i < names->length; i++) {
		char *name = names->elems[i];
		char *value = snailHashTableGet(regs,name);
		if (!snailIsInt(value)) {
			snailBuffer *msg = snailBufferCreate(16);
			snailBufferAddString(msg, "dos.int86: register ");
			snailBufferAddString(msg,name);
			snailBufferAddString(msg," has non-numeric value ");
			snailBufferAddString(msg,value);
			snailBufferAddChar(msg,0);
			snailSetResult(snail,msg->bytes);
			snailBufferDestroy(msg);
			snailArrayDestroy(names,free);
			snailHashTableDestroy(regs,free);
			return snailStatusError;
		}
		int32_t v = strtol(value,NULL,10);
		if (!snailDosSetReg(&r, name, v)) {
			snailBuffer *msg = snailBufferCreate(16);
			snailBufferAddString(msg, "dos.int86: unrecognised register '");
			snailBufferAddString(msg,name);
			snailBufferAddString(msg,"'");
			snailBufferAddChar(msg,0);
			snailSetResult(snail,msg->bytes);
			snailBufferDestroy(msg);
			snailArrayDestroy(names,free);
			snailHashTableDestroy(regs,free);
			return snailStatusError;
		}
	}
	snailArrayDestroy(names,free);
	snailHashTableDestroy(regs,free);
	if (__dpmi_int(vector,&r) != 0) {
		snailSetResult(snail,"dos.int86: __dpmi_int call failed");
		return snailStatusError;
	}
	snailHashTable *out = snailHashTableCreate(16);
	snailDosGetReg(&r, out);
	char *quoted = snailQuoteDict(out);
	snailHashTableDestroy(out,free);
	snailSetResult(snail, quoted);
	free(quoted);
	return snailStatusOk;
}

NATIVE(dos_mem_peek,2) {
	NATIVE_ARG_MUSTINT(0);
	NATIVE_ARG_MUSTINT(1);
	int32_t segment = strtol(args[0],NULL,10);
	int32_t offset = strtol(args[1],NULL,10);
	if (segment < 0 || segment > 0xFFFF) {
		snailSetResult(snail,"dos.mem.peek: segment out of range");
		return snailStatusError;
	}
	if (offset < 0 || offset > 0xFFFF) {
		snailSetResult(snail,"dos.mem.peek: offset out of range");
		return snailStatusError;
	}
	uint8_t byte;
	dosmemget(((segment<<4)+offset),1,&byte);
	snailSetResultInt(snail,byte);
	return snailStatusOk;
}

NATIVE(dos_reboot,0) {
	__dpmi_regs r;
	sync();
	memset(&r,0,sizeof(__dpmi_regs));
	r.x.cs = 0xffff;
	r.x.ip = 0x0000;
	__dpmi_simulate_real_mode_procedure_retf(&r);
	snailSetResult(snail,"Reboot failed");
	return snailStatusError;
}
