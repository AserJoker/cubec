class A {
public:
  virtual ~A() = default;
};
class B : public A {};
void test(A *a) {
  B *bb = dynamic_cast<B *>(a);
}