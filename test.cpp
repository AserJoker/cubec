auto get_fn() {
  int* a = 0;
  int* b = 0;
  return [a,b]() { return *a+*b; };
}
int run() {
  auto fn = get_fn();
  return fn();
}