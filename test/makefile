ALL=$(basename $(wildcard *.cpp))
define TEST_CASE
	@./$(1)
	@diff $(2) $(3)
	@echo "$(word 1,$(1))...\e[32mPass\e[0m"
	@rm $(2)
endef
all: $(ALL)
%: %.cpp
	g++ -Wall -std=c++17 -o $@ $<
.PHONY: test
test: $(ALL)
	$(call TEST_CASE,ColsoleLogtest > ColsoleLogtest.out,ColsoleLogtest.out,ColsoleLogoutput.std)
	$(call TEST_CASE,FileLogtest,FileLogTest.out,FileLogoutput.std)
	$(call TEST_CASE,DailyLogTest,$(shell date "+%Y-%m-%d"),FileLogoutput.std)
	$(call TEST_CASE,LoopFileLogTest,0.log,FileLogoutput.std)
.PHONY: clean
clean: clean/test clean/application
	
.PHONY: clean/test
clean/test:
	-rm .index .new
.PHONY: clean/application
clean/application: 
	-rm $(ALL)
