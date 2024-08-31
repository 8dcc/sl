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
;;   (defmacro 1+ (var)
;;     (list '+ 1 var))
;;
;; Will expand to:
;;
;;   (define 1+
;;     (macro (var)
;;       (list '+ 1 var)))
;;
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

;; Simle implmentation of `defun'. For example:
;;
;;     (defun my-function (n)
;;       (+ 1 n))
;;
;; Will expand to:
;;
;;     (define my-function
;;       (lambda (n)
;;         (+ 1 n)))
;;
(defmacro defun (name &rest lambda-args)
  (list 'define name
        (append 'lambda lambda-args)))

;; Define a simple function using the previous `defun' macro. Wrap the call in:
;;
;;   (macroexpand 'EXPR)
;;
;; To see how the macro expands.
(defun my-function (a b)
  (define unused 'not-returned)
  (+ a b 10))
(my-function 1 2)
