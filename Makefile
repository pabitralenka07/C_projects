CC       ?= gcc
CFLAGS   ?= -Wall -Wextra -Wpedantic -std=c11
LDFLAGS  ?=
SANFLAGS  = -fsanitize=address,undefined

# Projects in the portfolio
ALL_PROJECTS = 01_hospital_management 02_bus_reservation 03_airport_management \
                04_zoo_management 05_library_management 06_bank_management \
                07_simple_calculator 08_palindrome_checker 09_factorial_calculator \
                10_todo_list \
                11_rock_paper_scissors 12_currency_converter 13_atm_system \
                14_contact_management 15_dictionary_app 16_tic_tac_toe

# Active projects that have been implemented (with a Makefile)
ACTIVE_PROJECTS := $(foreach p,$(ALL_PROJECTS),$(if $(wildcard $(p)/Makefile),$(p),))

.PHONY: all test clean $(ALL_PROJECTS)

all: $(ACTIVE_PROJECTS)

$(ACTIVE_PROJECTS):
	$(MAKE) -C $@ CC="$(CC)" CFLAGS="$(CFLAGS)"

test:
	@for p in $(ACTIVE_PROJECTS); do \
		echo "=================================================="; \
		echo "=== Testing $$p ==="; \
		echo "=================================================="; \
		$(MAKE) -C $$p test CC="$(CC)" CFLAGS="$(CFLAGS)"; \
	done

clean:
	@for p in $(ACTIVE_PROJECTS); do \
		$(MAKE) -C $$p clean; \
	done
