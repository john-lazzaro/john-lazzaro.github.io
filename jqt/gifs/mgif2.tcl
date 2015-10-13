
catch {exec pstogif proxy2.ps proxy2.gif} message
if {$message != ""} {puts $message}

catch {exec giftrans -g#d8d8d8=#cccccc proxy2.gif > tmp.gif} message
if {$message != ""} {puts $message}

catch {exec mv tmp.gif proxy2.gif} message
if {$message != ""} {puts $message}

catch {exec cp proxy2.gif ../} message
if {$message != ""} {puts $message}
