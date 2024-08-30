;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - Macro definitions
;;   - Macro expansion
;;   - Macro calls
;;   - Usage of `defmacro' and `defun' macros.
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
;;   (define my-variable (+ my-variable 1))
(macroexpand '(inc my-variable))

;; Then the actual macro call, which is just the evaluated expansion
(inc my-variable)

;; Should be equivalent to the previous macro call
(eval (macroexpand '(inc my-variable)))

;; A messy implementation of `defun'. The whole:
;;
;;   (apply begin (quote ...))
;;
;; Part is needed because the body is a list of expressions, and we can't pass
;; that directly to a lambda.
(defmacro defun (name formals &rest body)
  (list 'define name
    (list 'lambda formals
      (list 'apply 'begin (list 'quote body)))))

;; Define a simple function using the previous `defun' macro. Wrap the call in:
;;
;;   (macroexpand 'EXPR)
;;
;; To see how the macro expands.
(defun my-function (a b)
  (+ a b 10))
(my-function 1 2)
