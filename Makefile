TEST_BIN = src/test_num2words

.PHONY: test clean-test screenshots

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): src/test_num2words.c src/num2words.c src/night_mode.c
	gcc -I src -o $@ $^

clean-test:
	rm -f $(TEST_BIN)

screenshots:
	bash scripts/take_screenshots.sh $(TIME)
