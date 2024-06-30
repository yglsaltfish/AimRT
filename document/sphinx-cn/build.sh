
mkdir -p $1/_build
mkdir -p $1/_static
mkdir -p $1/_templates

if command -v sphinx-build >/dev/null 2>&1; then
  if [ ! -d "$1/_build/html" ]; then
    rm -rf "$1/_build/html"
  fi
  sphinx-build -b html $1 $1/_build/html
fi