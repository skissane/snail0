/***---DOS: PROTOTYPES---***/
bool snailChannel_CONTROL_dosmem(snailChannel *channel, snailArray *cmdIn, char **resultOut);
char *snailChannel_CLOSE_dosmem(snailChannel *channel);
char *snailChannel_OPEN_dosmem(snailChannel *channel, void *driverArg);
char *snailChannel_READ_dosmem(snailChannel *channel, void *buf, size_t len, size_t *read);
char *snailChannel_WRITE_dosmem(snailChannel *channel, void *buf, size_t len, size_t *written);
void snailChannelSetup_DOSMEM(snailInterp *snail);
