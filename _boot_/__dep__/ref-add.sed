/^# Packages using this file: / {
  s/# Packages using this file://
  ta
  :a
  s/ holy / holy /
  tb
  s/ $/ holy /
  :b
  s/^/# Packages using this file:/
}
