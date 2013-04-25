

all: compile

compile:
	$(MAKE) -C publis

deploy: tidy
	rsync -av * gaelthomas@scm.gforge.inria.fr:/home/groups/vmkit2/htdocs/

tidy:
	find . \( -iname "*~" -o -iname "\#*" -o -iname ".DS_Store" \) -exec rm -f {} \;
