
catch {exec pstogif proxy.ps proxy.gif} message
if {$message != ""} {puts $message}

catch {exec giftrans -g#d8d8d8=#cccccc proxy.gif > tmp.gif} message
if {$message != ""} {puts $message}

catch {exec mv tmp.gif proxy.gif} message
if {$message != ""} {puts $message}

catch {exec cp proxy.gif ../} message
if {$message != ""} {puts $message}
