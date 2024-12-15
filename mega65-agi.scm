(define memories
  '((memory program
            (address (#x2001 . #x9fff)) (type any)
            (section (programStart #x2001) (startup #x200e)))
    (memory zeroPage (address (#x2 . #x7f)) (type ram) (qualifier zpage)
	    (section (registers #x2)))
    (memory stackPage (address (#x100 . #x1ff)) (type ram))
    (memory extradata
        (address (#x18000 . #x1f6ff)) (type bss) (qualifier far)
        (section extradata))
    (block heap (size #x0))
    ))
