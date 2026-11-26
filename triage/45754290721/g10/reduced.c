double logl(float);
void link_error() {
  if (logl(1.0L))
    link_error();
}
