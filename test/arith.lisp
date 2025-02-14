;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - Arithmetical primitives: `+', `-', `*', `/', `mod', `quotient',
;;     `remainder', `floor'
;;   - Bit-wise primitives: `bit-and', `bit-or', `bit-xor', `bit-not', `shr',
;;     `shl'
;;   - Type-conversion primitives: `int->flt', `flt->int'
;;------------------------------------------------------------------------------

(defun test-signed-integers (f)
  (list (f  9  2)
        (f  9 -2)
        (f -9  2)
        (f -9 -2)))

(defun test-signed-floats (f)
  (list (f  9.5  2.5)
        (f  9.5 -2.5)
        (f -9.5  2.5)
        (f -9.5 -2.5)))

(defun test-round-func (f)
  (list (list (f 5) (f 5.0) (f -5.0))
        (list (f  5.4) (f  5.5) (f  5.6))
        (list (f -5.4) (f -5.5) (f -5.6))))

(defun assert-type-conversion (val func inverse)
  (list val
        (func val)
        (inverse (func val))))

(defun assert-mod (dividend divisor)
  (equal? dividend
          (+ (mod dividend divisor)
             (* (floor (/ dividend divisor)) divisor))))

(defun assert-remainder (dividend divisor)
  (equal? dividend
          (+ (remainder dividend divisor)
             (* (quotient dividend divisor) divisor))))

(defun hex (num)
  (format "%x" num))

;;------------------------------------------------------------------------------

(list (+) (-) (*))

(+ 1 2 3 (- 3 4) (* 3 4))
(+ 1 2 3 (- 3 4) (* 3 4.0))

(assert-type-conversion 10     int->flt flt->int)
(assert-type-conversion 10.0   flt->int int->flt)
(assert-type-conversion 10     int->str str->int)
(assert-type-conversion 10.0   flt->str str->flt)
(assert-type-conversion "10"   str->int int->str)
(assert-type-conversion "10.0" str->flt flt->str)

(test-signed-floats +)
(test-signed-floats -)
(test-signed-floats *)
(test-signed-floats /)
(test-signed-floats mod)
(test-signed-floats assert-mod)

(test-signed-integers +)
(test-signed-integers -)
(test-signed-integers *)
(test-signed-integers quotient)
(test-signed-integers remainder)
(test-signed-integers assert-remainder)

(test-round-func round)
(test-round-func floor)
(test-round-func ceiling)
(test-round-func truncate)

(hex (bit-and 0x123456 0xFF00FF))
(hex (bit-or 0xFF00 0x00FF))
(hex (bit-xor 0x5555 0xFFFF))
(hex (bit-not 0xFF))
(hex (shr 0xFF00 8))
(hex (shl 0x00FF 8))
