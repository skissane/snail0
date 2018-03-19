/***---DOS: PROTOTYPES---***/
bool snailChannel_CONTROL_dosmem(snailChannel *channel, snailArray *cmdIn, char **resultOut);
bool snailDosSetReg(__dpmi_regs *regs, char *name, int32_t value);
char *snailChannel_CLOSE_dosmem(snailChannel *channel);
char *snailChannel_OPEN_dosmem(snailChannel *channel, void *driverArg);
char *snailChannel_READ_dosmem(snailChannel *channel, void *buf, size_t len, size_t *read);
char *snailChannel_WRITE_dosmem(snailChannel *channel, void *buf, size_t len, size_t *written);
void snailChannelSetup_DOSMEM(snailInterp *snail);
void snailDosGetReg(__dpmi_regs *regs, snailHashTable *out);
