;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - Macro definitions
;;   - Macro expansion
;;   - Macro calls
;;------------------------------------------------------------------------------

;; Define a macro called `defmacro'. It will take a `name', a list of `formals'
;; and a single `body' expression. For example:
;;
;;   (defmacro inc (var)
;;     (list '+ var 1))
;;
;; Will expand to:
;;
;;   (define inc
;;     (macro (var)
;;       (list '+ var 1)))
(define defmacro
  (macro (name formals body)
    (list 'define name (list 'macro formals body))))

;; Example usage of `defmacro'. Create a macro that will take a variable and
;; re-define it to its value plus one.
(defmacro inc (var)
  (list 'define var (list '+ var 1)))

(define my-variable 10)

;; First, test the `macroexpand' primitive, which should return:
;;   (define my-variable (+ my-variable 1.000000))
(macroexpand '(inc my-variable))

;; Then the actual macro call, which is just the evaluated expansion
(inc my-variable)

;; Should be equivalent to the previous macro call
(eval (macroexpand '(inc my-variable)))
