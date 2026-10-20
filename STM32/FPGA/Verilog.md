![alt text](image.png)
```verilog
module MUX21a (a,b,s,y);
  input a,b,s;
  output y;
  assign y=(s?a:b);
endmodule;
```