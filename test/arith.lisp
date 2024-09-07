;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - Arithmetical primitives: `+', `-', `*', `/', `mod'
;;   - Bit-wise primitives: `bit-and', `bit-or', `bit-xor', `bit-not', `shr',
;;     `shl'
;;------------------------------------------------------------------------------

'(+ 1 2 3 (- 3 4) (* 3 4))
(+ 1 2 3 (- 3 4) (* 3 4)) ; Expected: 17

(define test-signed-decimals
  (lambda (f)
    (list (f  9.5  2.5)
          (f  9.5 -2.5)
          (f -9.5  2.5)
          (f -9.5 -2.5))))

(test-signed-decimals +)   ; Expected: (12.0 7.0 -7.0 -12.0)
(test-signed-decimals -)   ; Expected: (7.0 12.0 -12.0 -7.0)
(test-signed-decimals *)   ; Expected: (23.75 -23.75 -23.75 23.75)
(test-signed-decimals /)   ; Expected: (3.8 -3.8 -3.8 3.8)
(test-signed-decimals mod) ; Expected: (2.00 -0.50 0.50 -2.00)

(bit-and 246720 16711935) ; Expected: 196800 (0x300C0)
(bit-or 65280 255)        ; Expected: 65535 (0xFFFF)
(bit-xor 21845 65535)     ; Expected: 43690 (0xAAAA)
(bit-not 255)             ; FIXME:    18446744073709551360 (0xFFFFFFFFFFFFFF00)
(shr 65280 8)             ; Expected: 255 (0x00FF)
(shl 255 8)               ; Expected: 65280 (0xFF00)
