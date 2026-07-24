# Example makefile exercised by `make test`. It uses only shell built-ins
# (echo, sleep, cp, cat, rm) so it runs on any Linux/WSL box without a compiler,
# yet it demonstrates every feature mmake supports:
#
#   - variables (recursive '=' and the automatic $@ $< $^)
#   - a .PHONY target ('all', 'clean') that always runs
#   - a dependency DAG:  all -> app -> {a.o, b.o, c.o} -> {a,b,c}.src
#   - parallelism: a.o/b.o/c.o are independent, so `-j3` builds them at once
#     (each "compile" sleeps 1s; three overlap in ~1s instead of ~3s)
#   - staleness: touch b.src and only b.o and app rebuild on the next run
#   - recipe prefixes: '@' silences the command echo

CP = cp

.PHONY: all clean

all: app

# $^ = all prerequisites (a.o b.o c.o); $@ = the target (app)
app: a.o b.o c.o
	@echo "==> link $^ -> $@"
	@cat $^ > $@

# $< = the first prerequisite (the .src); $@ = the .o
a.o: a.src
	@echo "==> compile $< -> $@"
	@sleep 1
	@$(CP) $< $@

b.o: b.src
	@echo "==> compile $< -> $@"
	@sleep 1
	@$(CP) $< $@

c.o: c.src
	@echo "==> compile $< -> $@"
	@sleep 1
	@$(CP) $< $@

clean:
	@rm -f a.o b.o c.o app
