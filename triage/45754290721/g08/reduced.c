bar1_N;
bar1() {
  sizeof **({
    int(*x)[][bar1_N] = bar1_N;
    x;
  });
}
