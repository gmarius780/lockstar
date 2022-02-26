import React, { useEffect, useCallback, useState } from 'react'
import { useHistory } from 'react-router-dom'
import Button from '@material-ui/core/Button'
import Typography from '@material-ui/core/Typography'
import CircularProgress from '@material-ui/core/CircularProgress'
import CheckIcon from '@material-ui/icons/Check'
import CancelIcon from '@material-ui/icons/Cancel'
import { Form, FormikProps, Formik, Field, FieldProps } from 'formik'
import { Theme, makeStyles, createStyles } from '@material-ui/core/styles'
import { green, red } from '@material-ui/core/colors'
import FormControl from '@material-ui/core/FormControl'
import InputLabel from '@material-ui/core/InputLabel'
import Select from '@material-ui/core/Select'
import MenuItem from '@material-ui/core/MenuItem'
import clsx from 'clsx'
import * as Yup from 'yup'
import { startCase } from 'lodash'

import TextField from '../inputs/text-field'
import useLocalStorage from '../../hooks/useLocalStorage'
import Snackbar from '../snackbar'

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

const deviceSchema = Yup.object().shape({
  pin: Yup.number().required('Required'),
  label: Yup.string().required('Required'),
  type: Yup.string().required('Required'),
})

interface Values {
  pin?: number
  label: string
  type: string
}

interface Device {
  pin: number
  label: string
  type: string
  status?: number
}

interface Props {
  device?: Device | null
}

interface AvailableDevices {
  [key: string]: number[]
}

export default ({ device }: Props) => {
  const classes = useStyles()
  const history = useHistory()
  const [serverBaseUrl] = useLocalStorage('serverBaseUrl')
  const [data, setData] = useState<AvailableDevices | null>(null)
  const [error, setError] = useState<string | null>(null)
  const [loading, setLoading] = useState(false)

  const fetchData = useCallback(async () => {
    setLoading(true)

    const data = await fetch(`${serverBaseUrl}/api/devices/available`)

    if (data.ok) {
      setData(await data.json())
    } else {
      setError(await data.text())
    }

    setLoading(false)
  }, [serverBaseUrl])

  useEffect(() => {
    fetchData()
  }, [fetchData])

  return (
    <>
      <Snackbar message={error} severity="error" onClose={() => setError(null)}/>
      {!error ? loading ? <CircularProgress /> : (
        <>
          <Typography variant="h5" className={classes.title} noWrap>
            Device Settings
          </Typography>

          <Formik
            initialValues={{
              pin: device?.pin,
              label: device?.label || '',
              type: device?.type || '',
            }}
            validationSchema={deviceSchema}
            onSubmit={async (values, { setSubmitting, setStatus }) => {
              const url = device ? `${serverBaseUrl}/api/devices/${device.pin}` : `${serverBaseUrl}/api/devices/`
              const method = device ? 'PUT' : 'POST'
              const result = await fetch(url, {
                method,
                headers: {
                  'Content-Type': 'application/json',
                },
                body: JSON.stringify(values),
              })

              if (result.ok) {
                const { pin }: Device = await result.json()
                setStatus('success')
                setTimeout(() => {
                  history.push(`/devices/view/${pin}`)
                }, 750)
              } else {
                // TODO: handle error
                // eslint-disable-next-line no-console
                console.log(await result.text())
                setStatus('error')
              }

              setSubmitting(false)
            }}
          >
            {({ isSubmitting, status, values, errors, initialValues }: FormikProps<Values>) => {
              const availableTypes = Object.keys(data || {})
              let availablePins: number[] = []
              if (values?.type && data) {
                availablePins = data[values.type] || []
              }

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

              return (
                <Form>
                  {!device && <Field name="type">
                    {({ field, meta: { error } }: FieldProps) => (
                      <FormControl
                        variant="outlined"
                        margin="dense"
                        fullWidth
                        required
                      >
                        <InputLabel htmlFor="type">Type</InputLabel>
                        <Select
                          label={'Type'}
                          {...field}
                          inputProps={{
                            id: 'type',
                          }}
                        >
                          {availableTypes.map(item => <MenuItem key={item} value={item}>{startCase(item)}</MenuItem>)}
                        </Select>
                      </FormControl>
                    )}
                  </Field>}
                  {!device && <Field name="pin">
                    {({ field, meta: { error } }: FieldProps) => (
                      <FormControl
                        variant="outlined"
                        margin="dense"
                        fullWidth
                        required
                      >
                        <InputLabel htmlFor="pin">Pin</InputLabel>
                        <Select
                          label={'Pin'}
                          {...field}
                          inputProps={{
                            id: 'pin',
                          }}
                        >
                          {availablePins.map(item => <MenuItem key={item} value={item}>{item}</MenuItem>)}
                        </Select>
                      </FormControl>
                    )}
                  </Field>}
                  <Field name="label">
                    {({ field, meta: { error } }: FieldProps) => (
                      <TextField
                        label={'Label'}
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
      ) : null}
    </>
  )
}
