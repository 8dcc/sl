;; Run the Guix shell in a container with:
;;   guix shell --container --emulate-fhs -m manifest.scm

(specifications->manifest
 (list
  ;; Compilation
  "coreutils"
  "gcc-toolchain"
  "make"
  ;; Optional compile-time dependencies
  "valgrind"
  ;; Dependencies for tests
  "bash"
  "diffutils"
  "sed"))
