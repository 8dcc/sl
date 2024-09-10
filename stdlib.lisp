;;==============================================================================
;; Standard Lisp library for SL.
;; https://github.com/8dcc/sl
;;
;; TODO:
;;   - Logical `not'
;;   - `nth'
;;   - `member' (return arg0 when car is arg1)
;;   - `member?' (is arg1 in arg0)
;;   - `length'
;;   - `let' (Macro using `lambda')
;;   - Number equality with `='
;;==============================================================================

;;------------------------------------------------------------------------------
;; General macros
;;------------------------------------------------------------------------------

;; (defmacro 1+ (var)  >  (define 1+
;;   (list '+ 1 var))  >    (macro (var)
;;                     >      (list '+ 1 var)))
(define defmacro
  (macro (name &rest macro-args)
    (list 'define name
          (cons 'macro macro-args))))

;; (defun my-function (n)  >  (define my-function
;;   (+ 1 n))              >    (lambda (n)
;;                         >      (+ 1 n)))
(defmacro defun (name &rest lambda-args)
  (list 'define name
        (cons 'lambda lambda-args)))

;;------------------------------------------------------------------------------
;; List-related functions
;;------------------------------------------------------------------------------

(defun null? (lst)
  (equal? lst nil))

(defun caar (lst) (car (car lst)))
(defun cadr (lst) (car (cdr lst)))
(defun cdar (lst) (cdr (car lst)))
(defun cddr (lst) (cdr (cdr lst)))
(defun caaar (lst) (car (car (car lst))))
(defun caadr (lst) (car (car (cdr lst))))
(defun cadar (lst) (car (cdr (car lst))))
(defun caddr (lst) (car (cdr (cdr lst))))
(defun cdaar (lst) (cdr (car (car lst))))
(defun cdadr (lst) (cdr (car (cdr lst))))
(defun cddar (lst) (cdr (cdr (car lst))))
(defun cdddr (lst) (cdr (cdr (cdr lst))))

;;------------------------------------------------------------------------------
;; Conditional macros
;;------------------------------------------------------------------------------

;; TODO: Support multiple expressions in clause without using begin?
;;
;; (cond (pred1 expr1)           >  (if pred1 (begin expr1)
;;       (pred2 expr2)           >    (if pred2 (begin expr2)
;;       (pred3 expr3a expr3b))  >      (if pred3 (begin expr3a expr3b)
;;                               >        nil)))
(defmacro cond (&rest clauses)
  (defun cond-lst (clauses)
    (if (null? clauses)
        nil
        (list 'if (caar clauses)
              (cons 'begin (cdar clauses))
              (cond-lst (cdr clauses)))))
  (cond-lst clauses))
