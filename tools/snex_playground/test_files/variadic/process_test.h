/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 112
  error: ""
  filename: "variadic/process_test"
END_TEST_DATA
*/

span<float, 16> c1 = { 4.0f };
span<float, 16> c2 = { 3.0f };



int main(int input)
{
    ProcessData data;// = { c1, c2 };
    
    data.data[0] = c1;
    data.data[1] = c2;

    float z = 0.0f;
    
    for(auto& ch: data.data)
    {
        for(auto& s: ch)
        {
            z += s;
        }
    }
    
	return z;
}

