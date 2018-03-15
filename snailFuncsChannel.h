/***---CHANNEL FUNCTIONS---***/
char * snailChannel_OPEN_stdio (snailChannel *channel, void *driverArg) {
	if (driverArg == NULL)
		return snailDupString("argument is null");
	channel->driverInfo = driverArg;
	return NULL;
}

char * snailChannel_READ_stdio (snailChannel *channel, void *buf, size_t len, size_t *read) {
	FILE *fh = channel->driverInfo;
	if (fh == NULL) {
		return snailDupString("channel is closed");
	}
	flockfile(fh);
	clearerr(fh);
	size_t rc = fread(buf, 1, len, fh);
	if (rc > 0) {
		funlockfile(fh);
		*read = rc;
		return NULL;
	}
	if (ferror(fh)) {
		clearerr(fh);
		funlockfile(fh);
		return snailDupString("I/O error reading on channel");
	}
	funlockfile(fh);
	*read = 0;
	return NULL;
}

char * snailChannel_WRITE_stdio (snailChannel *channel, void *buf, size_t len, size_t *written) {
	FILE *fh = channel->driverInfo;
	if (fh == NULL) {
		return snailDupString("channel is closed");
	}
	flockfile(fh);
	clearerr(fh);
	size_t rc = fwrite(buf, 1, len, fh);
	*written = rc;
	clearerr(fh);
	funlockfile(fh);
	return rc == len ? NULL : snailDupString("I/O error writing on channel");
}

void snailChannelDestroy(snailChannel *channel) {
	if (channel->driver->f_DESTROY != NULL)
		channel->driver->f_DESTROY(channel);
	free(channel->name);
	free(channel);
}

void snailChannelDriverDestroy(snailChannelDriver *driver) {
	free(driver->name);
	free(driver);
}

char *snailChannelDriverRegister(snailInterp *snail, snailChannelDriver *driver) {
	snailChannelDriver *existing = snailHashTableGet(snail->channelDrivers,driver->name);
	if (existing != NULL) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"already exists a channel driver named '");
		snailBufferAddString(msg,driver->name);
		snailBufferAddString(msg,"'");
		snailBufferAddChar(msg,0);
		char *r = snailDupString(msg->bytes);
		snailBufferDestroy(msg);
		return r;
	}
	snailHashTablePut(snail->channelDrivers,driver->name,driver);
	return NULL;
}

char * snailChannelRegister(snailInterp *snail, char *channelName, char *driverName, void *driverArg) {
	snailChannel *existing = snailHashTableGet(snail->channels,channelName);
	if (existing != NULL) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"already exists a channel named '");
		snailBufferAddString(msg,channelName);
		snailBufferAddString(msg,"'");
		snailBufferAddChar(msg,0);
		char *r = snailDupString(msg->bytes);
		snailBufferDestroy(msg);
		return r;
	}
	snailChannelDriver *driver = snailHashTableGet(snail->channelDrivers,driverName);
	if (driver == NULL) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"no such channel driver named '");
		snailBufferAddString(msg,driverName);
		snailBufferAddString(msg,"'");
		snailBufferAddChar(msg,0);
		char *r = snailDupString(msg->bytes);
		snailBufferDestroy(msg);
		return r;
	}
	snailChannel *channel = snailMalloc(sizeof(snailChannel));
	channel->name = snailDupString(channelName);
	channel->driver = driver;
	char *r_open = driver->f_OPEN(channel, driverArg);
	if (r_open != NULL) {
		free(channel->name);
		free(channel);
		return r_open;
	}
	snailHashTablePut(snail->channels,channelName,channel);
	return NULL;
}

void snailChannelSetup(snailInterp *snail) {
	// Setup STDIO driver
	snailChannelDriver * STDIO = snailMalloc(sizeof(snailChannelDriver));
	STDIO->name = snailDupString("STDIO");
	STDIO->f_OPEN = snailChannel_OPEN_stdio;
	STDIO->f_READ = snailChannel_READ_stdio;
	STDIO->f_WRITE = snailChannel_WRITE_stdio;
	STDIO->f_DESTROY = NULL;
	char *msg = snailChannelDriverRegister(snail,STDIO);
	if (msg != NULL)
		snailPanic(msg);

	// Register stdin/stdout/stderr channels
	msg = snailChannelRegister(snail,"stdin","STDIO",stdin);
	if (msg != NULL)
		snailPanic(msg);
	msg = snailChannelRegister(snail,"stdout","STDIO",stdout);
	if (msg != NULL)
		snailPanic(msg);
	msg = snailChannelRegister(snail,"stderr","STDIO",stderr);
	if (msg != NULL)
		snailPanic(msg);
}

char *snailChannelWrite(snailInterp *snail, char *channelName, void *buf, size_t len, size_t *written) {
	snailChannel *channel = snailHashTableGet(snail->channels,channelName);
	if (channel == NULL) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"no such channel named '");
		snailBufferAddString(msg,channelName);
		snailBufferAddString(msg,"'");
		snailBufferAddChar(msg,0);
		char *r = snailDupString(msg->bytes);
		snailBufferDestroy(msg);
		return r;
	}
	return channel->driver->f_WRITE(channel, buf, len, written);
}

char *snailChannelRead(snailInterp *snail, char *channelName, void *buf, size_t len, size_t *read) {
	snailChannel *channel = snailHashTableGet(snail->channels,channelName);
	if (channel == NULL) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"no such channel named '");
		snailBufferAddString(msg,channelName);
		snailBufferAddString(msg,"'");
		snailBufferAddChar(msg,0);
		char *r = snailDupString(msg->bytes);
		snailBufferDestroy(msg);
		return r;
	}
	return channel->driver->f_READ(channel, buf, len, read);
}
