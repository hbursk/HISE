/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "variadic/simple_variadic_test"
END_TEST_DATA
*/

struct X
{
    int v = 0;
    
    void process(int input)
    {
        v = input;
    }
};

container::chain<parameter::empty, X, X> c;

int main(int input)
{
    return 12;
}