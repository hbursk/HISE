/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 12
  output: 3
  error: ""
  filename: "dyn/dyn_wrap_2"
END_TEST_DATA
*/

span<float, 6> s = { 1.0f, 2.0f, 3.0f, 4.f, 5.0f, 6.f };
dyn<float> d;

dyn<float>::wrapped i;

void assign()
{
    d = s;
}

float main(float input)
{
    assign();
    
    return d[i] + d[i.moved(1)];
}

