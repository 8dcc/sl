;; Copyright 2024 8dcc
;;
;; This file is part of SL.
;;
;; This program is free software: you can redistribute it and/or modify it under
;; the terms of the GNU General Public License as published by the Free Software
;; Foundation, either version 3 of the License, or any later version.
;;
;; This program is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
;; FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
;; details.
;;
;; You should have received a copy of the GNU General Public License along with
;; SL. If not, see <https://www.gnu.org/licenses/>.
;;
;;==============================================================================
;;
;; Standard Lisp library for SL. <https://github.com/8dcc/sl>
;;
;; TODO:
;;   - `push', `pop'
;;   - `assoc'
;;   - `find'
;;   - `filter'
;;   - `reduce'
;;   - More mapping functions (See Maclisp):
;;       - `map'
;;       - `mapc'
;;       - `mapcdr'
;;   - `thread-last' (perhaps rename, check other lisps)
;;   - `memq' (return arg0 when car is `eq?' to arg1)
;;   - `member' (return arg0 when car is `equal?' to arg1)
;;   - `member?' (is arg1 in arg0?)
;;   - `string-split' using regex
;;   - See Emacs' "M-x shortdoc" categories.

;;------------------------------------------------------------------------------
;; Meta-definition macros
;;------------------------------------------------------------------------------

;; (defmacro 1+ (var)  >  (define 1+
;;   (list '+ 1 var))  >    (macro (var)
;;                     >      (list '+ 1 var)))
(define defmacro
  (macro (name &rest macro-args)
    `(define ,name (macro ,@macro-args))))

;; (defun my-function (n)  >  (define my-function
;;   (+ 1 n))              >    (lambda (n)
;;                         >      (+ 1 n)))
(defmacro defun (name &rest lambda-args)
  `(define ,name (lambda ,@lambda-args)))

;;------------------------------------------------------------------------------
;; List-accessing functions
;;------------------------------------------------------------------------------

(defun caar  (lst) (car (car lst)))
(defun cadr  (lst) (car (cdr lst)))
(defun cdar  (lst) (cdr (car lst)))
(defun cddr  (lst) (cdr (cdr lst)))
(defun caaar (lst) (car (car (car lst))))
(defun caadr (lst) (car (car (cdr lst))))
(defun cadar (lst) (car (cdr (car lst))))
(defun caddr (lst) (car (cdr (cdr lst))))
(defun cdaar (lst) (cdr (car (car lst))))
(defun cdadr (lst) (cdr (car (cdr lst))))
(defun cddar (lst) (cdr (cdr (car lst))))
(defun cdddr (lst) (cdr (cdr (cdr lst))))

(defun last (lst)
  (if (null? (cdr lst))
      (car lst)
      (last (cdr lst))))

;;------------------------------------------------------------------------------
;; List-building functions
;;------------------------------------------------------------------------------

(defun cons* (&rest args)
  (if (null? (cdr args))
      (car args)
      (cons (car args)
            (apply cons* (cdr args)))))

;;------------------------------------------------------------------------------
;; Conditional macros
;;------------------------------------------------------------------------------

;; (when pred  >  (if pred
;;   expr1     >    (begin
;;   expr2)    >      expr1
;;             >      expr2)
;;             >    nil)
(defmacro when (predicate &rest body)
  `(if ,predicate
       (begin ,@body)
       nil))

;; (unless pred  >  (when (not pred)
;;   expr1       >    expr1
;;   expr2)      >    expr2)
(defmacro unless (predicate &rest body)
  `(when (not ,predicate)
     ,@body))

;; TODO: Convert to primitive?
;;
;; (cond (pred1 expr1)           >  (if pred1 (begin expr1)
;;       (pred2 expr2)           >    (if pred2 (begin expr2)
;;       (pred3 expr3a expr3b))  >      (if pred3 (begin expr3a expr3b)
;;                               >        nil)))
(defmacro cond (&rest clauses)
  (if (null? clauses)
      nil
      (list 'if (caar clauses)
            (cons 'begin (cdar clauses))
            (cons 'cond (cdr clauses)))))

;;------------------------------------------------------------------------------
;; Local variables
;;------------------------------------------------------------------------------

;; (let ((s1 e1)   >  ((lambda (s1 s2 s3)
;;       (s2 e2)   >     body1
;;       (s3 e3))  >     body2
;;   body1         >     body3)
;;   body2         >   e1 e2 e3)
;;   body3)        >
(defmacro let (definitions &rest body)
  `((lambda ,(mapcar car definitions)
      ,@body)
    ,@(mapcar cadr definitions)))

;; (let* ((s1 e1)   >  (let ((s1 e1))
;;        (s2 e2)   >    (let ((s2 e2))
;;        (s3 e3))  >      (let ((s3 e3))
;;   body1          >        body1
;;   body2          >        body2
;;   body3)         >        body3)))
(defmacro let* (definitions &rest body)
  (if (null? definitions)
      `(begin ,@body)
      ;;`(quote ,(caar definitions))))
      `(let ((,(caar definitions) ,(cadar definitions)))
         (let* ,(cdr definitions)
           ,@body))))

;;------------------------------------------------------------------------------
;; General predicates
;;------------------------------------------------------------------------------

(defun not (predicate)
  (if predicate nil tru))

(defun null? (expr)
  (not expr))

(defun every (f lst)
  (cond ((null? lst) tru)
        ((not (f (car lst))) nil)
        (tru (every f (cdr lst)))))

;; TODO: Use `or'.
(defun some (f lst)
  (if (null? lst)
      nil
      (let ((result (f (car lst))))
        (if result result
            (some f (cdr lst))))))

;; NOTE: Should match C's `EXPRP_NUMBER'
(defun number? (expr)
  (or (int? expr)
      (flt? expr)))

;; NOTE: Should match C's `EXPRP_APPLICABLE'
(defun applicable? (expr)
  (or (primitive? expr)
      (lambda?    expr)
      (macro?     expr)))

;;------------------------------------------------------------------------------
;; Debugging
;;------------------------------------------------------------------------------

(defmacro assert (predicate)
  `(if ,predicate ,predicate
       (error (append "Assertion `"
                      (write-to-str (quote ,predicate))
                      "' failed."))))

;; TODO: Toggle by removing `func' if it's already in `*debug-trace*'
(defun trace (func)
  (assert (applicable? func))
  (define-global *debug-trace* (cons func *debug-trace*))
  "Trace enabled.")

;;------------------------------------------------------------------------------
;; Mapping
;;------------------------------------------------------------------------------

(defun mapcar (f lst)
  (if (null? lst)
      nil
      (cons (f (car lst))
            (mapcar f (cdr lst)))))

;;------------------------------------------------------------------------------
;; Math functions
;;------------------------------------------------------------------------------

(defun expt (b e)
  (defun iter (total e)
    (cond ((= e 0) 1)
          ((= e 1) total)
          (tru (iter (* total b) (- e 1)))))
  (iter b e))
