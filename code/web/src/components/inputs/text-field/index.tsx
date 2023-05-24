import React from 'react'
import TextField, { TextFieldProps } from '@material-ui/core/TextField'
import InputAdornment from '@material-ui/core/InputAdornment'
import ErrorIcon from '@material-ui/icons/ReportProblem'
import { Theme, createStyles, makeStyles } from '@material-ui/core/styles'

interface CustomInputProps extends Omit<TextFieldProps, 'error' | 'name'> {
  name: string
  error?: any
}

const useStyles = makeStyles((theme: Theme) => createStyles({
  errorIcon: {
    color: '#ff1818',
  },
}))

export default ({ error, ...props }: CustomInputProps) => {
  const classes = useStyles()

  return (
    <TextField
      {...props}
      fullWidth
      variant="outlined"
      margin="dense"
      error={!!error}
      helperText={error}
      InputProps={{
        endAdornment: error && (
          <InputAdornment position="end">
            <ErrorIcon className={classes.errorIcon}/>
          </InputAdornment>
        ),
      }}
    />
  )
}
