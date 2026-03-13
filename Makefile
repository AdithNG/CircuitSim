BUILD_DIR ?= build
CONFIG ?= Debug
PYTHON ?= python

.PHONY: configure build test run-dc run-tran run-ac plot python-smoke sweep-dc showcase clean

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR) --config $(CONFIG)

test: build
	ctest --test-dir $(BUILD_DIR) -C $(CONFIG) --output-on-failure

run-dc: build
	$(BUILD_DIR)/Debug/circuitsim_cli examples/resistor_divider.cir

run-tran: build
	$(BUILD_DIR)/Debug/circuitsim_cli tran examples/rc_charge.cir 1e-4 5e-3

run-ac: build
	$(BUILD_DIR)/Debug/circuitsim_cli ac examples/ac_lowpass.cir 159154.94309189535

plot: build
	$(PYTHON) python/plot_transient.py examples/rc_charge.cir 1e-4 5e-3 --output plots/rc_charge.png

python-smoke: build
	$(PYTHON) tests/python_bindings_smoke.py --module-dir build/python

sweep-dc: build
	$(PYTHON) python/sweep.py dc examples/resistor_divider_param.cir.in RLOAD 1000,2000,4000 --module-dir build/python --observe out --output-json build/dc_sweep.json --plot plots/dc_sweep.png

showcase: build
	$(PYTHON) python/chip_showcase.py --module-dir build/python

clean:
	cmake -E rm -rf $(BUILD_DIR)
