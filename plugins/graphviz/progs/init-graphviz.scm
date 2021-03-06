
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-graphviz.scm
;; DESCRIPTION : Initialize Graphviz plugin
;; COPYRIGHT   : (C) 2019 Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (graphviz-serialize lan t)
    (with u (pre-serialize lan t)
      (with s (texmacs->code (stree->tree u) "SourceCode")
        (string-append s "\n<EOF>\n"))))

(define (graphviz-launcher)
  (if (url-exists? "$TEXMACS_HOME_PATH/plugins/tmpy")
      (string-append "python \""
                     (getenv "TEXMACS_HOME_PATH")
                     "/plugins/tmpy/session/tm_graphviz.py\"")
      (string-append "python \""
                     (getenv "TEXMACS_PATH")
                     "/plugins/tmpy/session/tm_graphviz.py\"")))

(plugin-configure graphviz 
  (:require (url-exists-in-path? "dot"))
  (:launch ,(graphviz-launcher))
  (:serializer ,graphviz-serialize)
  (:session "Graphviz"))
