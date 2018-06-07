include Makefile

git-wrapper$(X): git-wrapper.o git.res
	$(QUIET_LINK)$(CC) $(ALL_LDFLAGS) $(COMPAT_CFLAGS) \
		-fno-stack-protector -Wall -o $@ $^ -lshell32 -lshlwapi

git-wrapper.o: %.o: ../%.c GIT-PREFIX
	$(QUIET_CC)$(CC) $(ALL_CFLAGS) $(COMPAT_CFLAGS) \
		-fno-stack-protector -o $*.o -c -Wall -Wwrite-strings $<

git-bash.res git-cmd.res git-wrapper.res gitk.res compat-bash.res: \
		%.res: ../%.rc
	$(QUIET_RC)$(RC) -i $< -o $@

git-bash.exe cmd/gitk.exe cmd/git-gui.exe: ALL_LDFLAGS += -mwindows

git-bash.exe git-cmd.exe compat-bash.exe: %.exe: %.res

cmd/gitk.exe cmd/git-gui.exe: gitk.res

git-bash.exe git-cmd.exe compat-bash.exe \
cmd/git.exe cmd/gitk.exe cmd/git-gui.exe: \
		%.exe: git-wrapper.o git.res
	@mkdir -p cmd
	$(QUIET_LINK)$(CC) $(ALL_LDFLAGS) $(COMPAT_CFLAGS) -o $@ $^ -lshlwapi

print-builtins:
	@echo $(BUILT_INS)

strip-all: strip
	$(STRIP) $(STRIP_OPTS) \
		contrib/credential/wincred/git-credential-wincred.exe \
		cmd/git{,-gui,k}.exe compat-bash.exe git-{bash,cmd,wrapper}.exe

install-pdbs:
	$(INSTALL) -d -m 755 '$(DESTDIR_SQ)$(bindir_SQ)'
	$(INSTALL) -d -m 755 '$(DESTDIR_SQ)$(gitexec_instdir_SQ)'
	$(INSTALL) -d -m 755 '$(DESTDIR_SQ)/cmd'
	$(INSTALL) -m 644 git.pdb '$(DESTDIR_SQ)$(bindir_SQ)'
	$(INSTALL) -m 644 $(patsubst %.exe,%.pdb,$(PROGRAMS)) \
		contrib/credential/wincred/git-credential-wincred.pdb \
		'$(DESTDIR_SQ)$(gitexec_instdir_SQ)'
	$(INSTALL) -m 644 cmd/git{,-gui,k}.pdb '$(DESTDIR_SQ)/cmd'
	$(INSTALL) -m 644 git-{bash,cmd,wrapper}.pdb '$(DESTDIR_SQ)'

sign-executables:
ifeq (,$(SIGNTOOL))
	@echo Skipping code-signing
else
	@eval $(SIGNTOOL) $(filter %.exe,$(ALL_PROGRAMS)) \
		contrib/credential/wincred/git-credential-wincred.exe git.exe \
		cmd/git{,-gui,k}.exe compat-bash.exe git-{bash,cmd,wrapper}.exe
endif