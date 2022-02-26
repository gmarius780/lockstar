import React, { SyntheticEvent, Fragment } from 'react'
import Snackbar from '@material-ui/core/Snackbar'
import IconButton from '@material-ui/core/IconButton'
import CloseIcon from '@material-ui/icons/Close'
import Alert from '@material-ui/lab/Alert'

interface Props {
  message?: string | null,
  severity: 'success' | 'info' | 'warning' | 'error' | undefined,
  onClose?: () => void,
}

export default ({ message, severity, onClose }: Props) => {
  const handleCloseNotification = (event?: SyntheticEvent, reason?: string) => {
    if (reason === 'clickaway') {
      return
    }

    onClose && onClose()
  }

  return (
    <>
      {message &&
        <Snackbar
          open={true}
          anchorOrigin={{ vertical: 'bottom', horizontal: 'left' }}
          autoHideDuration={6000}
          onClose={handleCloseNotification}
          action={
            <Fragment>
              <IconButton size="small" aria-label="close" color="inherit" onClick={handleCloseNotification}>
                <CloseIcon fontSize="small" />
              </IconButton>
            </Fragment>
          }
        >
          <Alert onClose={handleCloseNotification} severity={severity} elevation={6} variant="filled">
            {message}
          </Alert>
        </Snackbar>
      }
    </>
  )
}
