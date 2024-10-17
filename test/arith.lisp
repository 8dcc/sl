;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - Arithmetical primitives: `+', `-', `*', `/', `mod', `quotient',
;;     `remainder', `floor'
;;   - Bit-wise primitives: `bit-and', `bit-or', `bit-xor', `bit-not', `shr',
;;     `shl'
;;   - Type-conversion primitives: `int->flt', `flt->int'
;;------------------------------------------------------------------------------

(define test-signed-integers
  (lambda (f)
    (list (f  9  2)
          (f  9 -2)
          (f -9  2)
          (f -9 -2))))

(define test-signed-floats
  (lambda (f)
    (list (f  9.5  2.5)
          (f  9.5 -2.5)
          (f -9.5  2.5)
          (f -9.5 -2.5))))

(define assert-type-conversion
  (lambda (val func inverse)
    (list val
          (func val)
          (inverse (func val)))))

(define assert-mod
  (lambda (dividend divisor)
    (equal? dividend
            (+ (mod dividend divisor)
               (* (floor (/ dividend divisor)) divisor)))))

(define assert-remainder
  (lambda (dividend divisor)
    (equal? dividend
            (+ (remainder dividend divisor)
               (* (quotient dividend divisor) divisor)))))

;;------------------------------------------------------------------------------

(list (+) (-) (*)) ; Expected: (0 0 1)

(+ 1 2 3 (- 3 4) (* 3 4))   ; Expected: 17
(+ 1 2 3 (- 3 4) (* 3 4.0)) ; Expected: 17.0

(assert-type-conversion 10     int->flt flt->int)
(assert-type-conversion 10.0   flt->int int->flt)
(assert-type-conversion 10     int->str str->int)
(assert-type-conversion 10.0   flt->str str->flt)
(assert-type-conversion "10"   str->int int->str)
(assert-type-conversion "10.0" str->flt flt->str)

(test-signed-floats +)          ; Expected: (12.0 7.0 -7.0 -12.0)
(test-signed-floats -)          ; Expected: (7.0 12.0 -12.0 -7.0)
(test-signed-floats *)          ; Expected: (23.75 -23.75 -23.75 23.75)
(test-signed-floats /)          ; Expected: (3.8 -3.8 -3.8 3.8)
(test-signed-floats mod)        ; Expected: (2.00 -0.50 0.50 -2.00)
(test-signed-floats assert-mod) ; Expected: (tru tru tru tru)

(test-signed-integers +)                ; Expected: (11 7 -7 -11)
(test-signed-integers -)                ; Expected: (7 11 -11 -7)
(test-signed-integers *)                ; Expected: (18 -18 -18 18)
(test-signed-integers quotient)         ; Expected: (4 -4 -4 4)
(test-signed-integers remainder)        ; Expected: (1 1 -1 -1)
(test-signed-integers assert-remainder) ; Expected: (tru tru tru tru)

;; TODO: Add `format' primitive, wrap these in calls.
(bit-and 0x123456 0xFF00FF) ; Expected: 1179734 (0x120056)
(bit-or 0xFF00 0x00FF)      ; Expected: 65535 (0xFFFF)
(bit-xor 0x5555 0xFFFF)     ; Expected: 43690 (0xAAAA)
(bit-not 0xFF)              ; Expected: -256 (0xFFFFFFFFFFFFFF00)
(shr 0xFF00 8)              ; Expected: 255 (0x00FF)
(shl 0x00FF 8)              ; Expected: 65280 (0xFF00)
