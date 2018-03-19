/***---DOS NATIVES: IMPLEMENT---***/
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
	void *addr = (void*)((block->segment) << 4);
	memset(addr, 0, (block->paragraphs) << 4);
	char *channelName = snailChannelMakeName(snail);
        char *error = snailChannelRegister(snail, channelName, "DOSMEM", block);
	assert(error==NULL);
	snailSetResult(snail,channelName);
	free(channelName);
	return snailStatusOk;
}
