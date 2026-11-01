.PHONY: format lint check

format:
	find backend tests -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0 | xargs -0 clang-format -i
	cd frontend && npm run format

lint:
	find backend tests -type f -name '*.cpp' -print0 | xargs -0 -n 1 clang-tidy -p build
	cd frontend && npm run lint

check:
	pre-commit run --all-files
	cmake --build build --target BlazeTest
	cd build && ctest --output-on-failure
	cd frontend && npm run build
