
.PHONY: format format-cpp format-frontend format-config format-check

# Format all supported project files
format: format-cpp format-frontend format-config

# Format C and C++ files
format-cpp:
	find backend tests -type f \( \
		-name "*.cpp" -o \
		-name "*.hpp" -o \
		-name "*.h" -o \
		-name "*.cc" -o \
		-name "*.c" -o \
		-name "*.cxx" -o \
		-name "*.hxx" \
	\) -exec clang-format -i {} +

# Format frontend files
format-frontend:
	frontend/node_modules/.bin/prettier --write \
		"frontend/**/*.{ts,tsx,js,jsx,json,jsonc,css,html,md,yaml,yml}"

# Format configuration files
format-config:
	frontend/node_modules/.bin/prettier --write \
		--config frontend/.prettierrc.json \
		"frontend/.prettierrc.json" \
		".pre-commit-config.yaml"

# Check formatting without modifying files
format-check:
	find backend tests -type f \( \
		-name "*.cpp" -o \
		-name "*.hpp" -o \
		-name "*.h" -o \
		-name "*.cc" -o \
		-name "*.c" -o \
		-name "*.cxx" -o \
		-name "*.hxx" \
	\) -exec clang-format --dry-run --Werror {} +

	frontend/node_modules/.bin/prettier --check \
		--config frontend/.prettierrc.json \
		"frontend/**/*.{ts,tsx,js,jsx,json,jsonc,css,html,md,yaml,yml}" \
		"frontend/.prettierrc.json" \
		".pre-commit-config.yaml"
