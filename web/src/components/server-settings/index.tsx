import React from 'react'
import Button from '@material-ui/core/Button'
import Typography from '@material-ui/core/Typography'
import CircularProgress from '@material-ui/core/CircularProgress'
import CheckIcon from '@material-ui/icons/Check'
import CancelIcon from '@material-ui/icons/Cancel'
import { Form, FormikProps, Formik, Field, FieldProps } from 'formik'
import { Theme, makeStyles, createStyles } from '@material-ui/core/styles'
import { green, red } from '@material-ui/core/colors'
import clsx from 'clsx'
import * as Yup from 'yup'

import TextField from '../inputs/text-field'
import useLocalStorage from '../../hooks/useLocalStorage'

const useStyles = makeStyles((theme: Theme) => createStyles({
  title: {
    marginBottom: theme.spacing(1),
  },
  wrapper: {
    margin: theme.spacing(1),
    position: 'relative',
  },
  buttonSuccess: {
    backgroundColor: green[500],
    '&:hover': {
      backgroundColor: green[700],
    },
  },
  buttonError: {
    backgroundColor: red[500],
    '&:hover': {
      backgroundColor: red[700],
    },
  },
  buttonProgress: {
    color: green[500],
  },
}))

const schema = Yup.object().shape({
  serverBaseUrl: Yup.string().test({
    name: 'serverBaseUrl',
    // eslint-disable-next-line no-template-curly-in-string
    message: '${path} must be a valid URL',
    test: (value) => {
      try {
        if (value) {
          return new URL(value).protocol.startsWith('http')
        }
      } catch (error) {
        // do nothing
      }
      return false
    },
  }).required('Required'),
})

interface Values {
  serverBaseUrl: string
}

export default () => {
  const classes = useStyles()
  const [serverBaseUrl, setServerBaseUrl] = useLocalStorage('serverBaseUrl')

  return (
    <>
      <Typography variant="h5" className={classes.title} noWrap>
        Server Settings
      </Typography>

      <Formik
        initialValues={{
          serverBaseUrl: serverBaseUrl as string || '',
        }}
        validationSchema={schema}
        onSubmit={(values, { setSubmitting, setStatus }) => {
          // async simulation
          setTimeout(() => {
            try {
              setStatus('success')
              // @ts-ignore TS2349
              setServerBaseUrl(values.serverBaseUrl)
            } catch (error) {
              setStatus('error')
            }
            setSubmitting(false)
          }, 500)
        }}
      >
        {({ isSubmitting, status }: FormikProps<Values>) => {
          const buttonClassname = clsx({
            [classes.buttonSuccess]: status === 'success',
            [classes.buttonError]: status === 'error',
          })

          let buttonContent: any = 'Save'
          if (isSubmitting) {
            buttonContent = <CircularProgress size={24} className={classes.buttonProgress} />
          } else if (status === 'success') {
            buttonContent = <CheckIcon />
          } else if (status === 'error') {
            buttonContent = <CancelIcon />
          }

          return(
            <Form>
              <Field name="serverBaseUrl">
                {({ field, meta: { error } }: FieldProps) => (
                  <TextField
                    label={'Server Base Url'}
                    error={error}
                    {...field}
                    required
                  />
                )}
              </Field>

              <div className={classes.wrapper}>
                <Button
                  variant="contained"
                  color="primary"
                  type="submit"
                  className={buttonClassname}
                  disabled={isSubmitting}
                >
                  {buttonContent}
                </Button>
              </div>
            </Form>
          )
        }}
      </Formik>
    </>
  )
}
