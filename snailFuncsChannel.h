/***---CHANNEL SETUP ROUTINE---***/
void snailChannelSetup(snailInterp *snail) {
	snailChannelSetup_STDIO(snail);
	snailChannelSetup_DIRENT(snail);
#ifdef __DJGPP__
	snailChannelSetup_DOSMEM(snail);
#endif
}

/***---CHANNEL DRIVER FUNCTIONS---***/
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

/***---CHANNEL FUNCTIONS---***/
bool channelIsProtected(char *channelName) {
	if (strcmp(channelName,"stdin")==0) return true;
	if (strcmp(channelName,"stdout")==0) return true;
	if (strcmp(channelName,"stderr")==0) return true;
#ifdef __DJGPP__
	if (strcmp(channelName,"stdaux")==0) return true;
	if (strcmp(channelName,"stdprn")==0) return true;
#endif
	return false;
}

char * snailChannelClose(snailChannel *channel) {
	char *msg = NULL;
	if (channel->driver->f_CLOSE != NULL)
		msg = channel->driver->f_CLOSE(channel);
	free(channel->name);
	free(channel);
	return msg;
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

char *snailChannelWrite(snailInterp *snail, char *channelName, void *buf, size_t len, size_t *written) {
	snailChannel *channel = snailHashTableGet(snail->channels,channelName);
	if (channel == NULL) {
		return snailChannelNotFound(channelName);
	}
	if (channel->driver->f_WRITE == NULL) {
		return snailChannelNotSupported(channelName,"WRITE");
	}
	return channel->driver->f_WRITE(channel, buf, len, written);
}

char *snailChannelRead(snailInterp *snail, char *channelName, void *buf, size_t len, size_t *read) {
	snailChannel *channel = snailHashTableGet(snail->channels,channelName);
	if (channel == NULL) {
		return snailChannelNotFound(channelName);
	}
	if (channel->driver->f_READ == NULL) {
		return snailChannelNotSupported(channelName,"READ");
	}
	return channel->driver->f_READ(channel, buf, len, read);
}

char *snailChannelMakeName(snailInterp *snail) {
	snailBuffer *buf = snailBufferCreate(16);
	snailBufferAddChar(buf,'C');
	snailBufferAddI64(buf,++snail->autoId);
	snailBufferAddChar(buf,0);
	char *name = snailDupString(buf->bytes);
	snailBufferDestroy(buf);
	return name;
}

char *snailChannelFlush(snailInterp *snail, char *channelName) {
	snailChannel *channel = snailHashTableGet(snail->channels,channelName);
	if (channel == NULL) {
		return snailChannelNotFound(channelName);
	}
	// Treat FLUSH as no-op if not supported
	if (channel->driver->f_FLUSH == NULL)
		return NULL;
	return channel->driver->f_FLUSH(channel);
}

char *snailChannelGetLine(snailInterp *snail, char *channelName, char **bufOut) {
	snailChannel *channel = snailHashTableGet(snail->channels,channelName);
	if (channel == NULL) {
		return snailChannelNotFound(channelName);
	}
	if (channel->driver->f_GETLINE == NULL) {
		return snailChannelNotSupported(channelName,"GETLINE");
	}
	return channel->driver->f_GETLINE(channel, bufOut);
}

bool snailChannelControl(snailInterp *snail, char *channelName, snailArray *cmdIn, char **resultOut) {
	snailChannel *channel = snailHashTableGet(snail->channels,channelName);
	if (channel == NULL) {
		*resultOut = snailChannelNotFound(channelName);
		return false;
	}
	if (channel->driver->f_CONTROL == NULL) {
		*resultOut = snailChannelNotSupported(channelName,"CONTROL");
		return false;
	}
	return channel->driver->f_CONTROL(channel, cmdIn, resultOut);
}

/***---ERROR REPORTING FUNCTIONS---***/
char *snailChannelNotFound(char *channelName) {
	snailBuffer *msg = snailBufferCreate(16);
	snailBufferAddString(msg,"no such channel named '");
	snailBufferAddString(msg,channelName);
	snailBufferAddString(msg,"'");
	snailBufferAddChar(msg,0);
	char *r = snailDupString(msg->bytes);
	snailBufferDestroy(msg);
	return r;
}

char *snailChannelNotSupported(char *channelName, char *opName) {
	snailBuffer *msg = snailBufferCreate(16);
	snailBufferAddString(msg,"no such channel named '");
	snailBufferAddString(msg,channelName);
	snailBufferAddString(msg,"' does not support operation ");
	snailBufferAddString(msg,opName);
	snailBufferAddChar(msg,0);
	char *r = snailDupString(msg->bytes);
	snailBufferDestroy(msg);
	return r;
}

/***---IMPLEMENT STDIO DRIVER---***/
void snailChannelSetup_STDIO(snailInterp *snail) {
	// Setup STDIO driver
	snailChannelDriver * STDIO = snailMalloc(sizeof(snailChannelDriver));
	STDIO->name = snailDupString("STDIO");
	STDIO->f_OPEN = snailChannel_OPEN_stdio;
	STDIO->f_READ = snailChannel_READ_stdio;
	STDIO->f_WRITE = snailChannel_WRITE_stdio;
	STDIO->f_CLOSE = snailChannel_CLOSE_stdio;
	STDIO->f_FLUSH = snailChannel_FLUSH_stdio;
	STDIO->f_GETLINE = snailChannel_GETLINE_stdio;
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
#ifdef __DJGPP__
	msg = snailChannelRegister(snail,"stdprn","STDIO",stdprn);
	if (msg != NULL)
		snailPanic(msg);
	msg = snailChannelRegister(snail,"stdaux","STDIO",stdaux);
	if (msg != NULL)
		snailPanic(msg);
#endif
}

char * snailChannel_OPEN_stdio (snailChannel *channel, void *driverArg) {
	if (driverArg == NULL)
		return snailDupString("argument is null");
	channel->driverInfo = driverArg;
	return NULL;
}

char * snailChannel_GETLINE_stdio (snailChannel *channel, char **buf) {
	FILE *fh = channel->driverInfo;
	if (fh == NULL) {
		return snailDupString("channel is closed");
	}
	flockfile(fh);
	clearerr(fh);
	size_t n = 0;
	*buf = NULL;
	ssize_t rc = getline(buf, &n, fh);
	if (rc >= 0) {
		funlockfile(fh);
		return NULL;
	}
	free(*buf);
	*buf = NULL;
	if (ferror(fh)) {
		clearerr(fh);
		funlockfile(fh);
		return snailDupString("I/O error during GETLINE on STDIO channel");
	}
	funlockfile(fh);
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

char * snailChannel_FLUSH_stdio (snailChannel *channel) {
	FILE *fh = channel->driverInfo;
	if (fh == NULL) {
		return snailDupString("channel is closed");
	}
	flockfile(fh);
	clearerr(fh);
	int rc = fflush(fh);
	clearerr(fh);
	funlockfile(fh);
	return rc == 0 ? NULL : snailDupString("I/O error flushing channel");
}

char * snailChannel_CLOSE_stdio (snailChannel *channel) {
	FILE *fh = channel->driverInfo;
	if (fh == NULL) {
		return NULL;
	}
	if (fclose(fh) == 0)
		return NULL;
	int e = errno;
	snailBuffer *msg = snailBufferCreate(16);
	snailBufferAddString(msg,"OS error #");
	snailBufferAddI64(msg,e);
	snailBufferAddString(msg," while closing channel ");
	snailBufferAddString(msg,channel->name);
	snailBufferAddChar(msg,0);
	char *r = snailDupString(msg->bytes);
	snailBufferDestroy(msg);
	return r;
}

/***---IMPLEMENT DIRENT DRIVER---***/
void snailChannelSetup_DIRENT(snailInterp *snail) {
	// Setup DIRENT driver
	snailChannelDriver * DIRENT = snailMalloc(sizeof(snailChannelDriver));
	DIRENT->name = snailDupString("DIRENT");
	DIRENT->f_OPEN = snailChannel_OPEN_dirent;
	DIRENT->f_CLOSE = snailChannel_CLOSE_dirent;
	DIRENT->f_GETLINE = snailChannel_GETLINE_dirent;
	char *msg = snailChannelDriverRegister(snail,DIRENT);
	if (msg != NULL)
		snailPanic(msg);
}

char * snailChannel_OPEN_dirent (snailChannel *channel, void *driverArg) {
	if (driverArg == NULL)
		return snailDupString("argument is null");
	channel->driverInfo = driverArg;
	return NULL;
}

char * snailChannel_GETLINE_dirent (snailChannel *channel, char **buf) {
	*buf = NULL;
	DIR *fh = channel->driverInfo;
	if (fh == NULL) {
		return snailDupString("channel is closed");
	}
	errno = 0;
	struct dirent *entry = readdir(fh);
	int e = errno;
	if (entry == NULL) {
		if (e == 0) // No error, just end of directory
			return NULL;
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"OS error #");
		snailBufferAddI64(msg,e);
		snailBufferAddString(msg," reading from directory (GETLINE)");
		snailBufferAddChar(msg,0);
		char *r = snailDupString(msg->bytes);
		snailBufferDestroy(msg);
		return r;
	}
	*buf = snailDupString(entry->d_name);
	return NULL;
}

char * snailChannel_CLOSE_dirent (snailChannel *channel) {
	DIR *fh = channel->driverInfo;
	if (fh == NULL) {
		return NULL;
	}
	if (closedir(fh) == 0)
		return NULL;
	int e = errno;
	snailBuffer *msg = snailBufferCreate(16);
	snailBufferAddString(msg,"OS error #");
	snailBufferAddI64(msg,e);
	snailBufferAddString(msg," while closing channel ");
	snailBufferAddString(msg,channel->name);
	snailBufferAddChar(msg,0);
	char *r = snailDupString(msg->bytes);
	snailBufferDestroy(msg);
	return r;
}
