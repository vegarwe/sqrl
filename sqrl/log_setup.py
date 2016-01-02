import logging

def log_setup(verbose=False, logfilename=None, name=None):
    formatter = logging.Formatter('%(asctime)s %(levelname)8s: %(message)s')
    console = logging.StreamHandler()
    console.setFormatter(formatter)
    console.setLevel(logging.ERROR)
    if verbose:
        console.setLevel(logging.DEBUG)

    logger = logging.getLogger(name)
    logger.setLevel(logging.DEBUG)
    logger.addHandler(console)

    if logfilename is None:
        return
    logfile = logging.FileHandler(logfilename)
    logfile.setFormatter(formatter)
    logfile.setLevel(logging.INFO)
    if verbose:
        logfile.setLevel(logging.DEBUG)
    logger.addHandler(logfile)

